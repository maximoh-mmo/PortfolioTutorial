// Fill out your copyright notice in the Description page of Project Settings.


#include "Multiplayer/OnsetGameModeBase.h"

#include "Engine/Engine.h"
#include "Player/OnsetPlayerCharacter.h"

AOnsetGameModeBase::AOnsetGameModeBase()
{
	DefaultPawnClass = AOnsetPlayerCharacter::StaticClass();
}

void AOnsetGameModeBase::StartPlay()
{
	Super::StartPlay();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("GameMode Started!"));
	}
}