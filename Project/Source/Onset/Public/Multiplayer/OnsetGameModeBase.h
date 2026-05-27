// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "OnsetGameModeBase.generated.h"

UCLASS()
class ONSET_API AOnsetGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AOnsetGameModeBase();

	virtual void StartPlay() override;
};
