// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Combat/OnsetGameplayAbility.h"
#include "OnsetGA_Shadowstep.generated.h"

/** Shadowstep passive: on-kill blink behind nearest enemy within distance gate. */
UCLASS()
class ONSET_API UOnsetGA_Shadowstep : public UOnsetGameplayAbility
{
	GENERATED_BODY()

public:
	UOnsetGA_Shadowstep();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

private:
	/** Maximum distance to search for an enemy to blink to. */
	UPROPERTY(EditDefaultsOnly, Category = "Shadowstep")
	float DistanceGate = 1500.0f;

	/** Distance behind the target to blink to. */
	UPROPERTY(EditDefaultsOnly, Category = "Shadowstep")
	float BehindOffset = 200.0f;

	/** Duration of invulnerability after blink. */
	UPROPERTY(EditDefaultsOnly, Category = "Shadowstep")
	float InvulnerabilityDuration = 0.5f;

	/** Gameplay effect for invulnerability. */
	UPROPERTY(EditDefaultsOnly, Category = "Shadowstep")
	TSubclassOf<UGameplayEffect> InvulnerabilityEffectClass;

	/** Find nearest valid enemy within distance gate. Returns nullptr if none. */
	AActor* FindNearestEnemy(AActor* SourceActor, float MaxDistance) const;
};