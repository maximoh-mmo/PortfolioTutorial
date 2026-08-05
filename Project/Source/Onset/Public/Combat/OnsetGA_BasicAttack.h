// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Abilities/GameplayAbility.h"
#include "OnsetGA_BasicAttack.generated.h"

/** Basic attack ability: plays montage and applies damage to the targeted actor within range. */
UCLASS()
class ONSET_API UOnsetGA_BasicAttack : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UOnsetGA_BasicAttack();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

private:
	/** Gameplay effect applied as damage to the target. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/** Maximum range at which this ability can hit a target. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float AttackRange = 300.0f;

	/** Anim montage to play during attack. */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	class UAnimMontage* AttackMontage;

	/** Time into montage when damage is applied (seconds). */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	float DamageTime = 0.3f;

	/** Handle for the montage timer. */
	FTimerHandle MontageTimerHandle;

	void ApplyDamageAfterDelay(const FGameplayAbilitySpecHandle Handle,
							   const FGameplayAbilityActorInfo* ActorInfo,
							   const FGameplayAbilityActivationInfo ActivationInfo);
};
