// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Combat/OnsetGameplayAbility.h"
#include "OnsetGA_AoE.generated.h"

/** AoE ability: target-centered sphere overlap damage with PvP filtering. */
UCLASS()
class ONSET_API UOnsetGA_AoE : public UOnsetGameplayAbility
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
	/** Base damage applied to each target (physical). Supplied via SetByCaller to GE_GenericDamage. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float Damage = 25.0f;

	/** Radius of the AoE sphere. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float AoERadius = 300.0f;

	/** Collision channel for overlap detection. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TEnumAsByte<ECollisionChannel> OverlapChannel = ECC_GameTraceChannel1;
};