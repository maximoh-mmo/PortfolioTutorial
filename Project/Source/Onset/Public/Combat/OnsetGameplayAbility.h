// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Data/OnsetAbilityTypes.h"
#include "OnsetGameplayAbility.generated.h"

class UTexture2D;
class UAbilitySystemComponent;
class UOnsetCombatAttributeSet;

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
	 * Multiplier on the threat this ability's damage generates (1.0 = normal).
	 * Consumed by UOnsetAttributeSet when adding threat; derived abilities carry
	 * their DT_Abilities row value.
	 */
	virtual float GetThreatMultiplier() const { return 1.0f; }
	
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
	 * The source's total cooldown reduction fraction (0..MaxCDR). Phase 3 uses
	 * Haste% = AGI/(AGI+K_haste) with K_haste = 200; dual-wield and mastery CDR
	 * stack on top in later phases. Capped at 80% total.
	 */
	float GetTotalCooldownReduction() const;

	/**
	 * The base cooldown duration before CDR/multiplier scaling. Default returns the
	 * cooldown GE's static duration; derived classes (e.g. the basic attack) can
	 * override it with a weapon-archetype base so the attack rate follows the weapon.
	 */
	virtual float GetCooldownBaseDuration(const FGameplayAbilitySpecHandle Handle,
										  const FGameplayAbilityActorInfo* ActorInfo) const;
	/**
	 * Applies the shared GE_GenericDamage to TargetASC with the given element tag
	 * (Damage.Physical/Fire/Ice/Lightning/Poison) and SetByCaller magnitude. Damage
	 * values are never baked into the GE; they travel as SetByCaller on the spec and
	 * are resolved by UOnsetDamageExecution.
	 */
	void ApplyDamageToTarget(UAbilitySystemComponent* TargetASC,
							 FGameplayTag DamageTypeTag,
							 float Amount,
							 float Level) const;

	/** The avatar's combat attribute set (STR/INT/DEF/etc.), or null. */
	const UOnsetCombatAttributeSet* GetSourceCombatAttributes() const;

	/** The avatar's equipped WeaponBase (falls back to the class default weapon). */
	float GetSourceWeaponBase() const;

	/**
	 * Buff/debuff magnitude multiplier for the avatar. Support-class casters get
	 * SupportMasteryPotencyBonus (+20%): EffectiveBuffValue = Base x Potency.
	 * All other classes (and enemies) return 1.0.
	 */
	float GetBuffPotency() const;

	/**
	 * Applies the divisor-shaped scaling formula (combat-formulas §3):
	 * Raw = Base x (1 + Stat/100), where Stat = STR for Weapon scaling and INT for
	 * Skill scaling. Falls back to 1.0 when the avatar has no combat attribute set.
	 */
	float ResolveScaledBase(float Base, EOnsetScalingType ScalingType) const;
};
