#include "Subsystem/OnsetAuthSubsystem.h"

#include "Player/OnsetPlayerController.h"
#include "Player/OnsetPlayerState.h"
#include "Subsystem/OnsetPlayerDataSubsystem.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/World.h"

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

	UE_LOG(LogOnsetAuth, Log, TEXT("AuthSubsystem initialized: AuthMode=%s"), *AuthModeStr);
}

bool UOnsetAuthSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return HasAnyFlags(RF_ClassDefaultObject) || Outer ? true : false;
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

	FUniqueNetIdRepl UniqueId = PlayerState->GetUniqueId();
	if (UniqueId.IsValid())
	{
		PlatformID = UniqueId->ToString();
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
