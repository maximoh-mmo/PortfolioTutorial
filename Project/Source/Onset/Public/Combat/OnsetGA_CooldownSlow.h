// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Combat/OnsetGameplayAbility.h"
#include "OnsetGA_CooldownSlow.generated.h"

/** Debuff: applies OnsetCooldownSlowEffect to the current target, multiplying its cooldowns for a duration. */
UCLASS()
class ONSET_API UOnsetGA_CooldownSlow : public UOnsetGameplayAbility
{
	GENERATED_BODY()

public:
	UOnsetGA_CooldownSlow();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

private:
	/** How much the target's cooldowns are multiplied (2.0 = double cooldown time). */
	UPROPERTY(EditDefaultsOnly, Category = "Cooldown Slow")
	float CooldownRateMod = 2.0f;

	/** How long the slow lasts on the target. */
	UPROPERTY(EditDefaultsOnly, Category = "Cooldown Slow")
	float SlowDuration = 5.0f;

	/** Maximum distance to the target for the ability to connect. */
	UPROPERTY(EditDefaultsOnly, Category = "Cooldown Slow")
	float AbilityRange = 500.0f;

	/** Gameplay effect applied to the target. */
	UPROPERTY(EditDefaultsOnly, Category = "Cooldown Slow")
	TSubclassOf<UGameplayEffect> SlowEffectClass;
};
