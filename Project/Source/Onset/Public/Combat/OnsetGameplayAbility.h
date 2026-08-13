// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "OnsetGameplayAbility.generated.h"

class UTexture2D;
class UAbilitySystemComponent;

/**
 * Shared base for all combat abilities. Adds UI metadata (icon) and a helper
 * for reading the primary cooldown tag granted by this ability's cooldown GE,
 * so the ability bar can display assignable slots without hardcoding abilities.
 */
UCLASS()
class ONSET_API UOnsetGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	/** Icon displayed in ability bar slots when this ability is assigned. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability UI")
	TSoftObjectPtr<UTexture2D> AbilityIcon;

	/**
	 * Returns the first cooldown tag granted by this ability's cooldown GE
	 * (via GetCooldownTags), or an invalid tag if the ability has none.
	 */
	FGameplayTag GetPrimaryCooldownTag() const;
	
	/**
	 * Applies the cooldown GE scaled by the source character's CooldownMultiplier
	 * (Slow debuff => multiplier > 1 => longer cooldown). Falls back to the base
	 * behavior when the cooldown GE has no static duration.
	 */
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
							   const FGameplayAbilityActorInfo* ActorInfo,
							   const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	/**
	 * Applies the shared GE_GenericDamage to TargetASC with the given SetByCaller
	 * physical/magical magnitudes. Damage values are never baked into the GE; they
	 * travel as SetByCaller on the spec and are resolved by UOnsetDamageExecution.
	 */
	void ApplyDamageToTarget(UAbilitySystemComponent* TargetASC,
							 float Physical,
							 float Magical,
							 float Level) const;
};
