// Fill out your copyright notice in the Description page of Project Settings.


#include "Multiplayer/OnsetGameModeBase.h"

#include "Engine/Engine.h"
#include "Multiplayer/OnsetGameState.h"
#include "Player/OnsetPlayerCharacter.h"
#include "Player/OnsetPlayerController.h"
#include "Player/OnsetPlayerState.h"
#include "Subsystem/OnsetPlayerDataSubsystem.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogSteamAuth);

AOnsetGameModeBase::AOnsetGameModeBase()
{
	DefaultPawnClass = AOnsetPlayerCharacter::StaticClass();
	PlayerControllerClass = AOnsetPlayerController::StaticClass();
	GameStateClass = AOnsetGameState::StaticClass();
	HUDClass = nullptr;
}

void AOnsetGameModeBase::StartPlay()
{
	Super::StartPlay();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("GameMode Started!"));
	}
}

void AOnsetGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!NewPlayer || !HasAuthority())
		return;

	AOnsetPlayerState* PS = NewPlayer->GetPlayerState<AOnsetPlayerState>();
	if (!PS)
		return;

	// Platform ID defaults based on the network transport
	FString Platform = TEXT("Steam");
	FString PlatformID = TEXT("");

	// Extract platform-specific user ID from the network connection
	FUniqueNetIdRepl UniqueId = PS->GetUniqueId();
	if (UniqueId.IsValid())
	{
		PlatformID = UniqueId->ToString();
	}

	PS->PlayerPlatform = Platform;
	PS->PlayerPlatformID = PlatformID;

	UE_LOG(LogSteamAuth, Log, TEXT("PostLogin: player %s — platform=%s, id=%s"),
		*NewPlayer->GetName(), *Platform, *PlatformID);

	// Load or create persistent account data
	UOnsetPlayerDataSubsystem* DataSubsystem = GetWorld()->GetSubsystem<UOnsetPlayerDataSubsystem>();
	if (!DataSubsystem)
	{
		UE_LOG(LogSteamAuth, Warning, TEXT("PostLogin: UOnsetPlayerDataSubsystem not available"));
		return;
	}

	FOnsetAccountData AccountData;
	if (!DataSubsystem->LoadAccount(Platform, PlatformID, AccountData))
	{
		// No account found — auto-create on first login
		if (DataSubsystem->CreateAccount(Platform, PlatformID))
		{
			UE_LOG(LogSteamAuth, Log, TEXT("PostLogin: auto-created account for %s/%s"), *Platform, *PlatformID);
			// Reload the freshly-created account
			DataSubsystem->LoadAccount(Platform, PlatformID, AccountData);
		}
		else
		{
			UE_LOG(LogSteamAuth, Error, TEXT("PostLogin: failed to create account for %s/%s"), *Platform, *PlatformID);
			return;
		}
	}

	// Send account data to the client
	AOnsetPlayerController* PC = Cast<AOnsetPlayerController>(NewPlayer);
	if (PC)
	{
		PC->Client_AccountData(AccountData);
	}
}

void AOnsetGameModeBase::ValidateAuthTicket(APlayerController* NewPlayer, const FString& AuthTicket)
{
	if (!NewPlayer) return;

	if (AuthTicket.IsEmpty())
	{
		UE_LOG(LogSteamAuth, Error, TEXT("Steam auth failed — empty ticket from player, kicking."));
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

	UE_LOG(LogSteamAuth, Log, TEXT("Steam auth ticket accepted for player %s (%d chars)."),
		*NewPlayer->GetName(), AuthTicket.Len());
}