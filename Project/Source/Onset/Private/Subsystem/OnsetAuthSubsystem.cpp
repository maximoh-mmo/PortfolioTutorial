#include "Subsystem/OnsetAuthSubsystem.h"

#include "Data/OnsetSessionTokens.h"
#include "Player/OnsetPlayerCharacter.h"
#include "Player/OnsetPlayerController.h"
#include "Player/OnsetPlayerState.h"
#include "Subsystem/OnsetPlayerDataSubsystem.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/World.h"
#include "Misc/DateTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"
#include "OnlineSubsystemNames.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemSteam.h"
#include "OnlineAuthInterfaceUtilsSteam.h"


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

#if WITH_SERVER_CODE
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
#endif

	UE_LOG(LogOnsetAuth, Log, TEXT("AuthSubsystem initialized: AuthMode=%s, TokenLifetime=%ds"),
		*AuthModeStr, AuthTokenLifetimeSeconds);
	if (AuthMode == EOnsetAuthMode::Direct)
	{
		if (FOnlineSubsystemSteam* Steam = static_cast<FOnlineSubsystemSteam*>(IOnlineSubsystem::Get(STEAM_SUBSYSTEM)))
		{
			if (FOnlineAuthSteamUtilsPtr Utils = Steam->GetAuthInterfaceUtils(); Utils.IsValid() && Utils->IsSteamAuthEnabled())
			{
				Utils->OnAuthenticationResultDelegate.BindUObject(this, &UOnsetAuthSubsystem::OnSteamAuthenticationResult);
				bSteamAuthResultBound = true;
				UE_LOG(LogOnsetAuth, Log, TEXT("DirectAuth will wait for Steam packet-auth results."));
			}
		}
	}
}

void UOnsetAuthSubsystem::Deinitialize()
{
	if (bSteamAuthResultBound)
	{
		if (FOnlineSubsystemSteam* Steam = static_cast<FOnlineSubsystemSteam*>(IOnlineSubsystem::Get(STEAM_SUBSYSTEM)))
		{
			if (FOnlineAuthSteamUtilsPtr Utils = Steam->GetAuthInterfaceUtils();
				Utils.IsValid() && Utils->OnAuthenticationResultDelegate.IsBoundToObject(this))
				Utils->OnAuthenticationResultDelegate.Unbind();
		}
	}
	PendingSteamDirectAuth.Empty();
	EarlySteamAuthResults.Reset();
	bSteamAuthResultBound = false;
	Super::Deinitialize();
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
	UE_LOG(LogOnsetAuth, Log, TEXT("PreLogin: token validated for Address='%s' (%s/%s) slot=%d (pending=%d)"),
		*Address, *OutPlatform, *OutPlatformID, SlotIndex, PendingTokenAuthMap.Num());
	return {};
}

void UOnsetAuthSubsystem::PreLoginDirect(const FString& Options, const FString& Address)
{
	FString ClientIndexStr = UGameplayStatics::ParseOption(Options, TEXT("ClientIndex"));
	if (ClientIndexStr.IsEmpty())
	{
		return;
	}

	int32 ClientIndex = FCString::Atoi(*ClientIndexStr);
	PendingDevClientIndexMap.Add(Address, ClientIndex);
	UE_LOG(LogOnsetAuth, Log, TEXT("PreLoginDirect: captured ClientIndex=%d for %s"), ClientIndex, *Address);
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
		FPendingTokenAuth Pending;
		if (PendingTokenAuthMap.RemoveAndCopyValue(CleanAddress, Pending))
		{
			Platform = Pending.Platform;
			PlatformID = Pending.PlatformID;
			PlayerState->SelectedCharacterSlot = Pending.SlotIndex;
			UE_LOG(LogOnsetAuth, Log, TEXT("PostLogin: using token auth — %s/%s slot=%d"), *Platform, *PlatformID, Pending.SlotIndex);
		}
		else
		{
			UE_LOG(LogOnsetAuth, Warning, TEXT("PostLogin: NO pending token for CleanAddress='%s' (raw='%s'); pending=%d"),
				*CleanAddress, *Address, PendingTokenAuthMap.Num());
			for (const TPair<FString, FPendingTokenAuth>& PendingEntry : PendingTokenAuthMap)
			{
				UE_LOG(LogOnsetAuth, Warning, TEXT("PostLogin:   pending key='%s' (slot=%d)"),
					*PendingEntry.Key, PendingEntry.Value.SlotIndex);
			}
		}
	}
	else
	{
		FUniqueNetIdRepl UniqueId = PlayerState->GetUniqueId();
		if (UniqueId.IsValid())
		{
			if (UniqueId.GetType() == STEAM_SUBSYSTEM)
			{
				PlatformID = UniqueId->ToString();
			}
			else
			{
				// Null OSS (dev/test) — use a stable per-client dev ID if a ClientIndex was provided
				FString Address = NewPlayer->GetPlayerNetworkAddress();
				FString CleanAddress;
				Address.Split(TEXT(":"), &CleanAddress, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromStart);
				if (CleanAddress.IsEmpty()) CleanAddress = Address;
				if (int32* FoundIndex = PendingDevClientIndexMap.Find(CleanAddress))
				{
					PlatformID = FString::Printf(TEXT("%s-%s-C%d"), FPlatformProcess::ComputerName(), *FPlatformMisc::GetLoginId(), *FoundIndex);
					PendingDevClientIndexMap.Remove(CleanAddress);
					UE_LOG(LogOnsetAuth, Log, TEXT("PostLogin: dev identity for %s — %s"), *CleanAddress, *PlatformID);
				}
				else
				{
					PlatformID = UniqueId->ToString();
				}
			}
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
	if (PS && PS->GetUniqueId().IsValid())
	{
		const FString UserId = PS->GetUniqueId()->ToString();
		PendingSteamDirectAuth.Remove(UserId);
		EarlySteamAuthResults.Forget(UserId);
	}
	if (!PS || PS->SelectedCharacterSlot < 0) return;

	AOnsetPlayerController* PC = Cast<AOnsetPlayerController>(Exiting);
	if (!PC) return;

	// Route through the controller's exactly-once save so the Logout / PawnLeavingGame /
	// EndPlay disconnect paths flush a single write to the data store.
	PC->SaveCurrentCharacter(Exiting->GetPawn());
}

void UOnsetAuthSubsystem::ValidateAuthTicket(APlayerController* NewPlayer, const FString& AuthTicket)
{
	// Gameplay RPC ticket validation was replaced by the Steam packet handler. It uses
	// SteamGameServer::BeginAuthSession before gameplay RPCs are available and receives
	// Steam's asynchronous validation result. Retain this entry point temporarily for
	// source compatibility, but do not accept, persist, or log caller-provided tickets.
	UE_LOG(LogOnsetAuth, Warning, TEXT("Ignored deprecated gameplay Steam-ticket validation request."));
}

bool UOnsetAuthSubsystem::QueueSteamDirectAuth(APlayerController* NewPlayer)
{
	if (AuthMode != EOnsetAuthMode::Direct || !bSteamAuthResultBound || !NewPlayer)
		return false;
	if (const AOnsetPlayerState* PlayerState = NewPlayer->GetPlayerState<AOnsetPlayerState>())
	{
		const FUniqueNetIdRepl UniqueId = PlayerState->GetUniqueId();
		if (UniqueId.IsValid() && UniqueId.GetType() == STEAM_SUBSYSTEM)
		{
			bool bWasSuccess = false;
			if (EarlySteamAuthResults.Consume(UniqueId->ToString(), bWasSuccess))
			{
				if (bWasSuccess)
				{
					UE_LOG(LogOnsetAuth, Log, TEXT("PostLogin: Steam packet-auth already succeeded; continuing account login."));
					HandlePostLogin(NewPlayer);
				}
				else
				{
					UE_LOG(LogOnsetAuth, Warning, TEXT("PostLogin: Steam packet-auth already failed; account work was not started."));
				}
				return true;
			}
			PendingSteamDirectAuth.Add(UniqueId->ToString(), NewPlayer);
			UE_LOG(LogOnsetAuth, Log, TEXT("PostLogin: awaiting Steam packet-auth result for %s"), *NewPlayer->GetName());
			return true;
		}
	}
	return false;
}

void UOnsetAuthSubsystem::OnSteamAuthenticationResult(const FUniqueNetId& UserId, bool bWasSuccess)
{
	if (!UserId.IsValid()) return;
	TWeakObjectPtr<APlayerController> Player;
	if (!PendingSteamDirectAuth.RemoveAndCopyValue(UserId.ToString(), Player))
	{
		EarlySteamAuthResults.Record(UserId.ToString(), bWasSuccess);
		UE_LOG(LogOnsetAuth, Log, TEXT("Steam packet-auth result received before PostLogin (success=%d)."), bWasSuccess);
		return;
	}
	if (!bWasSuccess)
	{
		UE_LOG(LogOnsetAuth, Warning, TEXT("Steam packet-auth failed; account work was not started."));
		return;
	}
	if (Player.IsValid() && Player->HasAuthority())
	{
		UE_LOG(LogOnsetAuth, Log, TEXT("Steam packet-auth succeeded; continuing account login."));
		HandlePostLogin(Player.Get());
	}
}

FString UOnsetAuthSubsystem::GenerateToken(const FString& Platform, const FString& PlatformID, int32 SlotIndex)
{
#if WITH_SERVER_CODE
	const auto Result = Onset::SessionTokens::Issue({AuthTokenSecret, AuthTokenLifetimeSeconds},
		Platform, PlatformID, SlotIndex, FDateTime::UtcNow().ToUnixTimestamp());
	if (const auto* Token = Result.GetValue()) return *Token;
	UE_LOG(LogOnsetAuth, Warning, TEXT("GenerateToken: session token issuance failed"));
#endif
	return {};
}

bool UOnsetAuthSubsystem::ValidateToken(const FString& TokenStr, FString& OutPlatform, FString& OutPlatformID, int32& OutSlotIndex)
{
	OutPlatform.Empty();
	OutPlatformID.Empty();
	OutSlotIndex = -1;
#if WITH_SERVER_CODE
	const auto Result = Onset::SessionTokens::Validate({AuthTokenSecret, AuthTokenLifetimeSeconds},
		TokenStr, FDateTime::UtcNow().ToUnixTimestamp());
	if (const auto* Claims = Result.GetValue())
	{
		OutPlatform = Claims->Platform;
		OutPlatformID = Claims->PlatformID;
		OutSlotIndex = Claims->SlotIndex;
		return true;
	}
	UE_LOG(LogOnsetAuth, Warning, TEXT("ValidateToken: invalid or expired session token"));
#endif
	return false;
}
