// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/OnsetBaseCharacter.h"
class AOnsetPlayerController;
#include "GameFramework/CheatManager.h"
#include "OnsetCheatManager.generated.h"

/** Cheat manager for development testing */
UCLASS()
class ONSET_API UOnsetCheatManager : public UCheatManager
{
	GENERATED_BODY()
	
public:
	void God() override;
	
	UFUNCTION(exec)
	void Heal();
	const AController* GetController() const;
	AOnsetBaseCharacter* GetOnsetCharacter() const;
	UAbilitySystemComponent* GetAbilitySystemComponent() const;

	UPROPERTY()
	bool bGodMode = false;
};
