// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "OnsetCombatAttributeSet.generated.h"

/**
 * Combat attributes shared by player and NPC characters.
 *
 * Core stats (Strength, Intellect, Vitality, Defense, Agility, Luck) and per-element
 * resistances (Fire/Ice/Lightning/Poison) are the raw inputs consumed by the damage
 * pipeline in UOnsetDamageExecution and the ability scaling in UOnsetGameplayAbility.
 * Values are initialized to shared defaults here; the stat aggregation pass
 * (RecalculateDerivedStats, Phase 2) overrides them from class base + allocated
 * points + gear.
 *
 * CooldownMultiplier scales cooldown durations (base x CooldownMultiplier).
 * Default 1.0; the Slow debuff raises it above 1 via GE_GenericSlow
 * (UOnsetCooldownSlowEffect), so a slowed target's abilities recover more
 * slowly and it attacks less often.
 */
UCLASS()
class ONSET_API UOnsetCombatAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UOnsetCombatAttributeSet();

	// --- Core stats ---

	/** Weapon-scaled damage stat. Raw = WeaponBase x (1 + STR/100). */
	UPROPERTY(ReplicatedUsing=OnRep_Strength, EditAnywhere, BlueprintReadWrite, Category="Attributes")
	FGameplayAttributeData Strength;

	/** Skill-scaled damage stat. Raw = SkillBase x (1 + INT/100). */
	UPROPERTY(ReplicatedUsing=OnRep_Intellect, EditAnywhere, BlueprintReadWrite, Category="Attributes")
	FGameplayAttributeData Intellect;

	/** Vitality. Drives derived MaxHealth (stat aggregation, Phase 2). */
	UPROPERTY(ReplicatedUsing=OnRep_Vitality, EditAnywhere, BlueprintReadWrite, Category="Attributes")
	FGameplayAttributeData Vitality;

	/** Physical mitigation. Physical damage reduction = DEF/(DEF + K_DEF). */
	UPROPERTY(ReplicatedUsing=OnRep_Defense, EditAnywhere, BlueprintReadWrite, Category="Attributes")
	FGameplayAttributeData Defense;

	// --- Elemental resistances ---

	UPROPERTY(ReplicatedUsing=OnRep_ResistanceFire, EditAnywhere, BlueprintReadWrite, Category="Attributes")
	FGameplayAttributeData ResistanceFire;

	UPROPERTY(ReplicatedUsing=OnRep_ResistanceIce, EditAnywhere, BlueprintReadWrite, Category="Attributes")
	FGameplayAttributeData ResistanceIce;

	UPROPERTY(ReplicatedUsing=OnRep_ResistanceLightning, EditAnywhere, BlueprintReadWrite, Category="Attributes")
	FGameplayAttributeData ResistanceLightning;

	UPROPERTY(ReplicatedUsing=OnRep_ResistancePoison, EditAnywhere, BlueprintReadWrite, Category="Attributes")
	FGameplayAttributeData ResistancePoison;

	// --- Secondary stats ---

	/** Universal haste source (cooldown reduction) and future dodge. */
	UPROPERTY(ReplicatedUsing=OnRep_Agility, EditAnywhere, BlueprintReadWrite, Category="Attributes")
	FGameplayAttributeData Agility;

	/** Critical strike input (LUK curves in Phase 5). */
	UPROPERTY(ReplicatedUsing=OnRep_Luck, EditAnywhere, BlueprintReadWrite, Category="Attributes")
	FGameplayAttributeData Luck;

	/** Chance a shield blocks. Default 0 (no shield); gear + Tank mastery raise it. BlockDamageReduction = 50%. */
	UPROPERTY(ReplicatedUsing=OnRep_BlockChance, EditAnywhere, BlueprintReadWrite, Category="Attributes")
	FGameplayAttributeData BlockChance;

	/** Scales cooldown durations (base x CooldownMultiplier); Slow debuff raises it above 1. */
	UPROPERTY(ReplicatedUsing=OnRep_CooldownMultiplier, EditAnywhere, BlueprintReadWrite, Category="Attributes")
	FGameplayAttributeData CooldownMultiplier;

	// --- Buff/debuff aggregation (Phase 5) ---

	/** Additive outgoing damage fraction on the caster: Final = Base x (1 + OutgoingDamageMod). */
	UPROPERTY(ReplicatedUsing=OnRep_OutgoingDamageMod, EditAnywhere, BlueprintReadWrite, Category="Attributes")
	FGameplayAttributeData OutgoingDamageMod;

	/** Additive incoming damage fraction on the target: Final = Base x (1 + IncomingDamageMod). */
	UPROPERTY(ReplicatedUsing=OnRep_IncomingDamageMod, EditAnywhere, BlueprintReadWrite, Category="Attributes")
	FGameplayAttributeData IncomingDamageMod;

	/** Global outgoing multiplier = (1 + r)^N (default 1.0; N = prestige level). */
	UPROPERTY(ReplicatedUsing=OnRep_PrestigeMultiplier, EditAnywhere, BlueprintReadWrite, Category="Attributes")
	FGameplayAttributeData PrestigeMultiplier;

	ATTRIBUTE_ACCESSORS_BASIC(UOnsetCombatAttributeSet, Strength)
	ATTRIBUTE_ACCESSORS_BASIC(UOnsetCombatAttributeSet, Intellect)
	ATTRIBUTE_ACCESSORS_BASIC(UOnsetCombatAttributeSet, Vitality)
	ATTRIBUTE_ACCESSORS_BASIC(UOnsetCombatAttributeSet, Defense)
	ATTRIBUTE_ACCESSORS_BASIC(UOnsetCombatAttributeSet, ResistanceFire)
	ATTRIBUTE_ACCESSORS_BASIC(UOnsetCombatAttributeSet, ResistanceIce)
	ATTRIBUTE_ACCESSORS_BASIC(UOnsetCombatAttributeSet, ResistanceLightning)
	ATTRIBUTE_ACCESSORS_BASIC(UOnsetCombatAttributeSet, ResistancePoison)
	ATTRIBUTE_ACCESSORS_BASIC(UOnsetCombatAttributeSet, Agility)
	ATTRIBUTE_ACCESSORS_BASIC(UOnsetCombatAttributeSet, Luck)
	ATTRIBUTE_ACCESSORS_BASIC(UOnsetCombatAttributeSet, BlockChance)
	ATTRIBUTE_ACCESSORS_BASIC(UOnsetCombatAttributeSet, CooldownMultiplier)
	ATTRIBUTE_ACCESSORS_BASIC(UOnsetCombatAttributeSet, OutgoingDamageMod)
	ATTRIBUTE_ACCESSORS_BASIC(UOnsetCombatAttributeSet, IncomingDamageMod)
	ATTRIBUTE_ACCESSORS_BASIC(UOnsetCombatAttributeSet, PrestigeMultiplier)

	/**
	 * Returns the mitigation attribute that applies to the given damage tag:
	 * Defense for Physical, the matching Resistance for Fire/Ice/Lightning/Poison,
	 * and an invalid attribute for unknown tags.
	 */
	FGameplayAttribute GetMitigationAttributeForDamageTag(const FGameplayTag& DamageType) const;

	/** Re-initializes every attribute to its constructor default (pool return). */
	void ResetToDefaults();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	virtual void OnRep_Strength(const FGameplayAttributeData& OldStrength);

	UFUNCTION()
	virtual void OnRep_Intellect(const FGameplayAttributeData& OldIntellect);

	UFUNCTION()
	virtual void OnRep_Vitality(const FGameplayAttributeData& OldVitality);

	UFUNCTION()
	virtual void OnRep_Defense(const FGameplayAttributeData& OldDefense);

	UFUNCTION()
	virtual void OnRep_ResistanceFire(const FGameplayAttributeData& OldResistanceFire);

	UFUNCTION()
	virtual void OnRep_ResistanceIce(const FGameplayAttributeData& OldResistanceIce);

	UFUNCTION()
	virtual void OnRep_ResistanceLightning(const FGameplayAttributeData& OldResistanceLightning);

	UFUNCTION()
	virtual void OnRep_ResistancePoison(const FGameplayAttributeData& OldResistancePoison);

	UFUNCTION()
	virtual void OnRep_Agility(const FGameplayAttributeData& OldAgility);

	UFUNCTION()
	virtual void OnRep_Luck(const FGameplayAttributeData& OldLuck);

	UFUNCTION()
	virtual void OnRep_BlockChance(const FGameplayAttributeData& OldBlockChance);

	UFUNCTION()
	virtual void OnRep_CooldownMultiplier(const FGameplayAttributeData& OldCooldownMultiplier);

	UFUNCTION()
	virtual void OnRep_OutgoingDamageMod(const FGameplayAttributeData& OldOutgoingDamageMod);

	UFUNCTION()
	virtual void OnRep_IncomingDamageMod(const FGameplayAttributeData& OldIncomingDamageMod);

	UFUNCTION()
	virtual void OnRep_PrestigeMultiplier(const FGameplayAttributeData& OldPrestigeMultiplier);
};