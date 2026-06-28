// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "GameFramework/GameModeBase.h"
#include "OnsetGameModeBase.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSteamAuth, Log, All);

/** Default game mode. Sets the default pawn class OnsetPlayerCharacter and the player controller class. */
UCLASS()
class ONSET_API AOnsetGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AOnsetGameModeBase();

	virtual void StartPlay() override;

	void ValidateAuthTicket(APlayerController* NewPlayer, const FString& AuthTicket);
};
