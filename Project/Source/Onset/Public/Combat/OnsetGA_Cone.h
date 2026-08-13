// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Combat/OnsetGameplayAbility.h"
#include "OnsetGA_Cone.generated.h"

/** Cone ability: directional frontal cone overlap damage with PvP filtering. */
UCLASS()
class ONSET_API UOnsetGA_Cone : public UOnsetGameplayAbility
{
	GENERATED_BODY()

public:
	UOnsetGA_Cone();

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

	/** Angle of the cone in degrees. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float ConeHalfAngle = 90.0f;
	
	/** Range of the cone. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float ConeRange = 500.0f;

	/** Collision channel for overlap detection. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TEnumAsByte<ECollisionChannel> OverlapChannel = ECC_GameTraceChannel1;
};