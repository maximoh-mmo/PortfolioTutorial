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
	
	/** Fully heal the controlled character. */
	UFUNCTION(exec)
	void Heal();
	/** Applies the cooldown-slow debuff to the controlled character (CooldownMultiplier x RateMod for Duration seconds). */
	UFUNCTION(exec)
	void ApplyCooldownSlow(float RateMod = 2.0f, float Duration = 5.0f);
	/** Returns the active controller (player or AI) regardless of possession state. */
	const AController* GetController() const;
	/** Returns the controlled base character. */
	AOnsetBaseCharacter* GetOnsetCharacter() const;
	/** Returns the ability system component of the controlled character. */
	UAbilitySystemComponent* GetAbilitySystemComponent() const;

	UPROPERTY()
	bool bGodMode = false;
};
