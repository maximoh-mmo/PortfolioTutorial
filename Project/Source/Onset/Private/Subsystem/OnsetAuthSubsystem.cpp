#include "Subsystem/OnsetAuthSubsystem.h"

#include "Player/OnsetPlayerController.h"
#include "Player/OnsetPlayerState.h"
#include "Subsystem/OnsetPlayerDataSubsystem.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/World.h"
#include "Misc/Base64.h"
#include "Misc/DateTime.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogOnsetAuth);

void UOnsetAuthSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FString AuthModeStr;
	if (GConfig->GetString(TEXT("Onset.Auth"), TEXT("AuthMode"), AuthModeStr, GGameIni))
	{
		if (AuthModeStr.Equals(TEXT("Token"), ESearchCase::IgnoreCase))
		{
			AuthMode = EOnsetAuthMode::Token;
		}
	}

	FString Secret;
	if (GConfig->GetString(TEXT("Onset.Auth"), TEXT("AuthTokenSecret"), Secret, GGameIni))
	{
		AuthTokenSecret = Secret;
	}
	if (AuthTokenSecret.IsEmpty())
	{
		AuthTokenSecret = TEXT("default-dev-secret-change-me");
		UE_LOG(LogOnsetAuth, Warning, TEXT("AuthTokenSecret not configured — using dev default"));
	}

	int32 Lifetime = 0;
	if (GConfig->GetInt(TEXT("Onset.Auth"), TEXT("AuthTokenLifetimeSeconds"), Lifetime, GGameIni))
	{
		AuthTokenLifetimeSeconds = FMath::Max(Lifetime, 30);
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

	if (!ValidateToken(Token, OutPlatform, OutPlatformID))
	{
		return TEXT("Invalid or expired session token");
	}

	PendingTokenAuthMap.Add(Address, {OutPlatform, OutPlatformID});
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
		if (FPendingTokenAuth* Pending = PendingTokenAuthMap.Find(Address))
		{
			Platform = Pending->Platform;
			PlatformID = Pending->PlatformID;
			PendingTokenAuthMap.Remove(Address);
			UE_LOG(LogOnsetAuth, Log, TEXT("PostLogin: using token auth — %s/%s"), *Platform, *PlatformID);
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
	if (PC && PlayerState->SelectedCharacterSlot < 0)
	{
		PC->Client_AccountData(AccountData);

		if (AuthMode == EOnsetAuthMode::Token)
		{
			FString Token = GenerateToken(Platform, PlatformID);
			if (!Token.IsEmpty())
			{
				PC->Client_SessionToken(Token);
				UE_LOG(LogOnsetAuth, Log, TEXT("PostLogin: sent session token to %s"), *NewPlayer->GetName());
			}
		}
	}
	else if (PC && PlayerState->SelectedCharacterSlot >= 0)
	{
		UE_LOG(LogOnsetAuth, Log, TEXT("PostLogin: player %s already has slot %d (zone travel)"),
			*NewPlayer->GetName(), PlayerState->SelectedCharacterSlot);
	}
}

void UOnsetAuthSubsystem::HandleLogout(AController* Exiting)
{
	AOnsetPlayerState* PS = Exiting ? Exiting->GetPlayerState<AOnsetPlayerState>() : nullptr;
	if (PS && PS->SelectedCharacterSlot >= 0)
	{
		UOnsetPlayerDataSubsystem* DataSubsystem = GetWorld()->GetSubsystem<UOnsetPlayerDataSubsystem>();
		if (DataSubsystem)
		{
			DataSubsystem->SaveCharacter(PS->PlayerPlatform, PS->PlayerPlatformID, FOnsetFullCharacterData());
		}
	}
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
		FGenericPlatformMisc::GetSHA256Signature(K.GetData(), K.Num(), Hash);
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
	FGenericPlatformMisc::GetSHA256Signature(InnerInput.GetData(), InnerInput.Num(), InnerHash);

	TArray<uint8> OuterInput;
	OuterInput.Append(oPad);
	OuterInput.Append(InnerHash.Signature, 32);

	FSHA256Signature OuterHash;
	FGenericPlatformMisc::GetSHA256Signature(OuterInput.GetData(), OuterInput.Num(), OuterHash);

	TArray<uint8> Result;
	Result.Append(OuterHash.Signature, 32);
	return Result;
}

FString UOnsetAuthSubsystem::GenerateToken(const FString& Platform, const FString& PlatformID)
{
	if (AuthTokenSecret.IsEmpty()) return {};

	int64 NowUnix = FDateTime::UtcNow().ToUnixTimestamp();
	int64 ExpiryUnix = NowUnix + AuthTokenLifetimeSeconds;

	FString PayloadStr = FString::Printf(TEXT("%s|%s|%lld"), *PlatformID, *Platform, ExpiryUnix);
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

bool UOnsetAuthSubsystem::ValidateToken(const FString& TokenStr, FString& OutPlatform, FString& OutPlatformID)
{
	OutPlatform.Empty();
	OutPlatformID.Empty();

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
	if (Parts.Num() != 3)
	{
		UE_LOG(LogOnsetAuth, Warning, TEXT("ValidateToken: payload has %d parts (expected 3)"), Parts.Num());
		return false;
	}

	OutPlatformID = Parts[0];
	OutPlatform = Parts[1];
	int64 ExpiryUnix = FCString::Atoi64(*Parts[2]);

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
