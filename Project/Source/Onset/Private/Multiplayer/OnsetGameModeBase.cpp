// Fill out your copyright notice in the Description page of Project Settings.


#include "Multiplayer/OnsetGameModeBase.h"

#include "Engine/Engine.h"
#include "Multiplayer/OnsetGameState.h"
#include "Player/OnsetPlayerCharacter.h"
#include "Player/OnsetPlayerController.h"

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