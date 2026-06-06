// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OnsetBaseCharacter.h"
#include "OnsetPlayerController.h"
#include "GameFramework/CheatManager.h"
#include "OnsetCheatManager.generated.h"

/**
 * 
 */
UCLASS()
class ONSET_API UOnsetCheatManager : public UCheatManager
{
	GENERATED_BODY()
	
public:
	void God() override;
	
	UFUNCTION(exec)
	void Heal();
	AOnsetPlayerController* GetOnsetPlayerController() const;
	AOnsetBaseCharacter* GetOnsetCharacter() const;
	UAbilitySystemComponent* GetAbilitySystemComponent() const;

	bool bGodMode = false;
};
