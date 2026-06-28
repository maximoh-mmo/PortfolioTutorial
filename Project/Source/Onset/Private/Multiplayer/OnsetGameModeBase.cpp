// Fill out your copyright notice in the Description page of Project Settings.


#include "Multiplayer/OnsetGameModeBase.h"

#include "Engine/Engine.h"
#include "Multiplayer/OnsetGameState.h"
#include "Player/OnsetPlayerCharacter.h"
#include "Player/OnsetPlayerController.h"
#include "Player/OnsetPlayerState.h"

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