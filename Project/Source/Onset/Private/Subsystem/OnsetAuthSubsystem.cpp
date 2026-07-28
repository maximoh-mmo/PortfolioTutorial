#include "Subsystem/OnsetAuthSubsystem.h"

#include "Crypto/SHA256.h"
#include "Player/OnsetPlayerCharacter.h"
#include "Player/OnsetPlayerController.h"
#include "Player/OnsetPlayerState.h"
#include "Subsystem/OnsetPlayerDataSubsystem.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/World.h"
#include "Misc/Base64.h"
#include "Misc/DateTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"


DEFINE_LOG_CATEGORY(LogOnsetAuth);

void UOnsetAuthSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FString AuthModeStr;
	GConfig->GetString(TEXT("Onset.Auth"), TEXT("AuthMode"), AuthModeStr, GEngineIni);

	// Command line override: -AuthMode=Token
	FString CmdLineAuthMode;
	if (FParse::Value(FCommandLine::Get(), TEXT("AuthMode="), CmdLineAuthMode))
	{
		AuthModeStr = CmdLineAuthMode;
	}
	if (AuthModeStr.Equals(TEXT("Token"), ESearchCase::IgnoreCase))
	{
		AuthMode = EOnsetAuthMode::Token;
	}
	else if (AuthModeStr.Equals(TEXT("Direct"), ESearchCase::IgnoreCase))
	{
		AuthMode = EOnsetAuthMode::Direct;
	}

	FString Secret;
	GConfig->GetString(TEXT("Onset.Auth"), TEXT("AuthTokenSecret"), Secret, GEngineIni);
	if (!Secret.IsEmpty())
	{
		AuthTokenSecret = Secret;
	}
	if (AuthTokenSecret.IsEmpty())
	{
		AuthTokenSecret = TEXT("default-dev-secret-change-me");
		UE_LOG(LogOnsetAuth, Warning, TEXT("AuthTokenSecret not configured — using dev default"));
	}

	int32 Lifetime = 0;
	GConfig->GetInt(TEXT("Onset.Auth"), TEXT("AuthTokenLifetimeSeconds"), Lifetime, GEngineIni);
	if (Lifetime >= 30)
	{
		AuthTokenLifetimeSeconds = Lifetime;
	}

	UE_LOG(LogOnsetAuth, Log, TEXT("AuthSubsystem initialized: AuthMode=%s, TokenLifetime=%ds"),
		*AuthModeStr, AuthTokenLifetimeSeconds);
}

bool UOnsetAuthSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return HasAnyFlags(RF_ClassDefaultObject) || Outer != nullptr;
}

FString UOnsetAuthSubsystem::PreLoginTokenAuth(const FString& Options, const FString& Address, FString& OutPlatform, FString& OutPlatformID)
{
	OutPlatform.Empty();
	OutPlatformID.Empty();

	if (AuthMode != EOnsetAuthMode::Token)
	{
		return {};
	}

	FString Token = UGameplayStatics::ParseOption(Options, TEXT("Token"));
	if (Token.IsEmpty())
	{
		return TEXT("Missing session token");
	}

	int32 SlotIndex = -1;
	if (!ValidateToken(Token, OutPlatform, OutPlatformID, SlotIndex))
	{
		return TEXT("Invalid or expired session token");
	}

	PendingTokenAuthMap.Add(Address, {OutPlatform, OutPlatformID, SlotIndex});
	UE_LOG(LogOnsetAuth, Log, TEXT("PreLogin: token validated for %s (%s/%s)"), *Address, *OutPlatform, *OutPlatformID);
	return {};
}

void UOnsetAuthSubsystem::HandlePostLogin(APlayerController* NewPlayer)
{
	if (!NewPlayer || !NewPlayer->HasAuthority())
		return;

	AOnsetPlayerState* PlayerState = NewPlayer->GetPlayerState<AOnsetPlayerState>();
	if (!PlayerState)
		return;

	FString Platform = TEXT("Steam");
	FString PlatformID = TEXT("");

	if (AuthMode == EOnsetAuthMode::Token)
	{
		FString Address = NewPlayer->GetPlayerNetworkAddress();
		FString CleanAddress;
		Address.Split(TEXT(":"), &CleanAddress, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromStart);
		if (CleanAddress.IsEmpty()) CleanAddress = Address;
		if (FPendingTokenAuth* Pending = PendingTokenAuthMap.Find(CleanAddress))
		{
			Platform = Pending->Platform;
			PlatformID = Pending->PlatformID;
			PlayerState->SelectedCharacterSlot = Pending->SlotIndex;
			PendingTokenAuthMap.Remove(Address);
			UE_LOG(LogOnsetAuth, Log, TEXT("PostLogin: using token auth — %s/%s slot=%d"), *Platform, *PlatformID, Pending->SlotIndex);
		}
		else
		{
			// Token not found in pending map — might have arrived via PostLogin directly (LoginServer)
		}
	}
	else
	{
		FUniqueNetIdRepl UniqueId = PlayerState->GetUniqueId();
		if (UniqueId.IsValid())
		{
			PlatformID = UniqueId->ToString();
		}
		if (PlatformID.IsEmpty())
		{
			FString Address = NewPlayer->GetPlayerNetworkAddress();
			FString CleanAddress;
			Address.Split(TEXT(":"), &CleanAddress, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromStart);
			if (CleanAddress.IsEmpty()) CleanAddress = Address;
			PlatformID = TEXT("DEV_") + CleanAddress;
			UE_LOG(LogOnsetAuth, Log, TEXT("PostLogin: no unique ID — using network address as PlatformID (%s)"), *PlatformID);
		}
	}

	PlayerState->PlayerPlatform = Platform;
	PlayerState->PlayerPlatformID = PlatformID;

	UE_LOG(LogOnsetAuth, Log, TEXT("PostLogin: player %s — platform=%s, id=%s"),
		*NewPlayer->GetName(), *Platform, *PlatformID);

	UOnsetPlayerDataSubsystem* DataSubsystem = GetWorld()->GetSubsystem<UOnsetPlayerDataSubsystem>();
	if (!DataSubsystem)
	{
		UE_LOG(LogOnsetAuth, Warning, TEXT("PostLogin: UOnsetPlayerDataSubsystem not available"));
		return;
	}

	FOnsetAccountData AccountData;
	if (!DataSubsystem->LoadAccount(Platform, PlatformID, AccountData))
	{
		if (DataSubsystem->CreateAccount(Platform, PlatformID))
		{
			UE_LOG(LogOnsetAuth, Log, TEXT("PostLogin: auto-created account for %s/%s"), *Platform, *PlatformID);
			DataSubsystem->LoadAccount(Platform, PlatformID, AccountData);
		}
		else
		{
			UE_LOG(LogOnsetAuth, Error, TEXT("PostLogin: failed to create account for %s/%s"), *Platform, *PlatformID);
			return;
		}
	}

	AOnsetPlayerController* PC = Cast<AOnsetPlayerController>(NewPlayer);
	if (PC)
	{
		PC->Client_AccountData(AccountData);

		if (AuthMode == EOnsetAuthMode::Token)
		{
			FString Token = GenerateToken(Platform, PlatformID, PlayerState->SelectedCharacterSlot);
			if (!Token.IsEmpty())
			{
				PC->Client_SessionToken(Token);
				UE_LOG(LogOnsetAuth, Log, TEXT("PostLogin: sent session token to %s"), *NewPlayer->GetName());
			}
		}
	}
}

void UOnsetAuthSubsystem::HandleLogout(AController* Exiting)
{
	AOnsetPlayerState* PS = Exiting ? Exiting->GetPlayerState<AOnsetPlayerState>() : nullptr;
	if (!PS || PS->SelectedCharacterSlot < 0) return;

	AOnsetPlayerCharacter* PlayerChar = Exiting ? Cast<AOnsetPlayerCharacter>(Exiting->GetPawn()) : nullptr;

	if (!PlayerChar) return;

	UOnsetPlayerDataSubsystem* DataSubsystem = GetWorld()->GetSubsystem<UOnsetPlayerDataSubsystem>();
	if (!DataSubsystem) return;

	FOnsetFullCharacterData CharData;
	CharData.SlotIndex = PS->SelectedCharacterSlot;
	CharData.SavedPosition = PlayerChar->GetActorLocation();
	CharData.SavedRotationYaw = PlayerChar->GetActorRotation().Yaw;
	CharData.CurrentZone = GetWorld()->GetMapName();
	CharData.InventoryJSON = TEXT("{}");
	CharData.EquipmentJSON = TEXT("{}");
	CharData.QuestsJSON = TEXT("{}");

	DataSubsystem->SaveCharacter(PS->PlayerPlatform, PS->PlayerPlatformID, CharData);
}

void UOnsetAuthSubsystem::ValidateAuthTicket(APlayerController* NewPlayer, const FString& AuthTicket)
{
	if (!NewPlayer) return;

	if (AuthTicket.IsEmpty())
	{
		UE_LOG(LogOnsetAuth, Error, TEXT("Steam auth failed — empty ticket from player, kicking."));
		NewPlayer->Destroy();
		return;
	}

	AOnsetPlayerController* PC = Cast<AOnsetPlayerController>(NewPlayer);
	if (PC)
	{
		PC->ClearAuthTimeout();
		PC->Client_ClearAuthTimeout();
	}

	AOnsetPlayerState* PS = NewPlayer->GetPlayerState<AOnsetPlayerState>();
	if (PS)
	{
		PS->SteamAuthTicket = AuthTicket;
	}

	UE_LOG(LogOnsetAuth, Log, TEXT("Steam auth ticket accepted for player %s (%d chars)."),
		*NewPlayer->GetName(), AuthTicket.Len());
}

TArray<uint8> UOnsetAuthSubsystem::HmacSha256(const TArray<uint8>& Key, const TArray<uint8>& Data)
{
	constexpr int32 BlockSize = 64;
	TArray<uint8> K = Key;

	if (K.Num() > BlockSize)
	{
		FSHA256Signature Hash;
		
		FSHA256::GetSHA256Signature(K.GetData(), K.Num(), Hash);
		K.Empty();
	K.Append(Hash.Signature, 32);
	}

	while (K.Num() < BlockSize)
	{
		K.Add(0);
	}

	TArray<uint8> iPad;
	iPad.SetNum(BlockSize);
	TArray<uint8> oPad;
	oPad.SetNum(BlockSize);
	for (int32 i = 0; i < BlockSize; ++i)
	{
		iPad[i] = K[i] ^ 0x36;
		oPad[i] = K[i] ^ 0x5C;
	}

	TArray<uint8> InnerInput;
	InnerInput.Append(iPad);
	InnerInput.Append(Data);

	FSHA256Signature InnerHash;
	FSHA256::GetSHA256Signature(InnerInput.GetData(), InnerInput.Num(), InnerHash);

	TArray<uint8> OuterInput;
	OuterInput.Append(oPad);
	OuterInput.Append(InnerHash.Signature, 32);

	FSHA256Signature OuterHash;
	FSHA256::GetSHA256Signature(OuterInput.GetData(), OuterInput.Num(), OuterHash);

	TArray<uint8> Result;
	Result.Append(OuterHash.Signature, 32);
	return Result;
}

FString UOnsetAuthSubsystem::GenerateToken(const FString& Platform, const FString& PlatformID, int32 SlotIndex)
{
	if (AuthTokenSecret.IsEmpty()) return {};

	int64 NowUnix = FDateTime::UtcNow().ToUnixTimestamp();
	int64 ExpiryUnix = NowUnix + AuthTokenLifetimeSeconds;

	FString PayloadStr = FString::Printf(TEXT("%s|%s|%d|%lld"), *PlatformID, *Platform, SlotIndex, ExpiryUnix);
	FTCHARToUTF8 PayloadConv(*PayloadStr);
	TArray<uint8> PayloadBytes;
	PayloadBytes.Append(reinterpret_cast<const uint8*>(PayloadConv.Get()), PayloadConv.Length());
	FString PayloadB64 = FBase64::Encode(PayloadBytes);

	FTCHARToUTF8 KeyConv(*AuthTokenSecret);
	TArray<uint8> KeyBytes;
	KeyBytes.Append(reinterpret_cast<const uint8*>(KeyConv.Get()), KeyConv.Length());
	TArray<uint8> SigBytes = HmacSha256(KeyBytes, PayloadBytes);
	FString SigB64 = FBase64::Encode(SigBytes);

	return PayloadB64 + TEXT(".") + SigB64;
}

bool UOnsetAuthSubsystem::ValidateToken(const FString& TokenStr, FString& OutPlatform, FString& OutPlatformID, int32& OutSlotIndex)
{
	OutPlatform.Empty();
	OutPlatformID.Empty();
	OutSlotIndex = -1;

	if (TokenStr.IsEmpty()) return false;

	FString PayloadB64, SigB64;
	if (!TokenStr.Split(TEXT("."), &PayloadB64, &SigB64))
	{
		UE_LOG(LogOnsetAuth, Warning, TEXT("ValidateToken: malformed token (no separator)"));
		return false;
	}

	TArray<uint8> PayloadBytes;
	if (!FBase64::Decode(PayloadB64, PayloadBytes))
	{
		UE_LOG(LogOnsetAuth, Warning, TEXT("ValidateToken: failed to decode payload"));
		return false;
	}

	PayloadBytes.Add(0);
	FString PayloadStr(UTF8_TO_TCHAR(PayloadBytes.GetData()));

	TArray<FString> Parts;
	PayloadStr.ParseIntoArray(Parts, TEXT("|"));
	if (Parts.Num() != 4)
	{
		UE_LOG(LogOnsetAuth, Warning, TEXT("ValidateToken: payload has %d parts (expected 4)"), Parts.Num());
		return false;
	}

	OutPlatformID = Parts[0];
	OutPlatform = Parts[1];
	OutSlotIndex = FCString::Atoi(*Parts[2]);
	int64 ExpiryUnix = FCString::Atoi64(*Parts[3]);

	if (ExpiryUnix < FDateTime::UtcNow().ToUnixTimestamp())
	{
		UE_LOG(LogOnsetAuth, Warning, TEXT("ValidateToken: token expired (exp=%lld, now=%lld)"),
			ExpiryUnix, FDateTime::UtcNow().ToUnixTimestamp());
		return false;
	}

	FTCHARToUTF8 KeyConv(*AuthTokenSecret);
	TArray<uint8> KeyBytes;
	KeyBytes.Append(reinterpret_cast<const uint8*>(KeyConv.Get()), KeyConv.Length());

	PayloadBytes.Pop(); // remove null terminator before re-hashing
	TArray<uint8> ExpectedSig = HmacSha256(KeyBytes, PayloadBytes);

	TArray<uint8> ActualSig;
	if (!FBase64::Decode(SigB64, ActualSig) || ExpectedSig != ActualSig)
	{
		UE_LOG(LogOnsetAuth, Warning, TEXT("ValidateToken: signature mismatch"));
		return false;
	}

	return true;
}
