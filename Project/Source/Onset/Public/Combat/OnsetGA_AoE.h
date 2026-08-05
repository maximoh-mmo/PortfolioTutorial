// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "OnsetGA_AoE.generated.h"

/** AoE ability: target-centered sphere overlap damage with PvP filtering. */
UCLASS()
class ONSET_API UOnsetGA_AoE : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UOnsetGA_AoE();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

private:
	/** Gameplay effect applied as damage to targets in radius. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/** Radius of the AoE sphere. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float AoERadius = 300.0f;

	/** Collision channel for overlap detection. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TEnumAsByte<ECollisionChannel> OverlapChannel = ECC_GameTraceChannel1;
};