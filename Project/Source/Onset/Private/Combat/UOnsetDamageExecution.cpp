// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/UOnsetDamageExecution.h"

#include "AbilitySystemComponent.h"
#include "Combat/OnsetAbilityLibrary.h"
#include "Combat/OnsetEquipmentLibrary.h"
#include "Core/OnsetBaseCharacter.h"
#include "Data/OnsetAbilityTypes.h"
#include "Data/OnsetEquipmentTypes.h"
#include "GAS/OnsetAttributeSet.h"
#include "GAS/OnsetCombatAttributeSet.h"
#include "GAS/OnsetGameplayTags.h"

UOnsetDamageExecution::UOnsetDamageExecution()
{
	// Attributes are read directly from the target's attribute sets inside
	// Execute_Implementation (same pattern as the Health output), so no capture
	// definitions are needed here. The SetByCaller seam never changes.
	RelevantAttributesToCapture.Empty();
}

void UOnsetDamageExecution::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	if (!TargetASC)
	{
		return;
	}

	// Invulnerability gate: TAG_State_Invulnerable (e.g. Shadowstep) negates all
	// damage. Emitting no modifier keeps the rest of the pipeline (hit reaction,
	// death, threat, noise) from seeing a change, matching the old attribute-set gate.
	if (TargetASC->HasMatchingGameplayTag(TAG_State_Invulnerable))
	{
		return;
	}

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	// ---- Damage pipeline (v2, see combat-formulas §5) ----
	// Each element travels as its own SetByCaller magnitude; the target mitigates
	// each element by its matching attribute (DEF for Physical, RES_x otherwise).
	const UOnsetCombatAttributeSet* TargetCombat = TargetASC->GetSet<UOnsetCombatAttributeSet>();

	// Source set (caster) for the LUK crit curves and the outgoing damage mod.
	const UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	const UOnsetCombatAttributeSet* SourceCombat = SourceASC ? SourceASC->GetSet<UOnsetCombatAttributeSet>() : nullptr;

	constexpr float BlockDamageReduction = 0.50f;

	// Zone-tier K scaling: K_DEF / K_elem x KZoneTierScale (tuning seam, default 1.0).
	const float K_Defense = 100.0f * UOnsetEquipmentLibrary::GetZoneTierKScale();
	const float K_Elemental = 80.0f * UOnsetEquipmentLibrary::GetZoneTierKScale();

	// LUK crit curves (combat-formulas §14):
	//   CritChance   = 5% + LUK/(LUK+K_Crit),    cap 70%
	//   CritMult     = 1.5 + LUK/(LUK+K_CritMult) x 2.5, cap 4.0x
	constexpr float BaseCritChance = 0.05f;
	constexpr float MaxCritChance = 0.70f;
	constexpr float BaseCritMultiplier = 1.50f;
	constexpr float MaxCritMultiplier = 4.00f;
	constexpr float K_Crit = 200.0f;
	constexpr float K_CritMultiplier = 400.0f;

	// Step 1: Raw = sum of all element magnitudes, then ±15% variance.
	float Raw = 0.0f;
	const FGameplayTag ElementTags[] = {
		TAG_Damage_Physical,
		TAG_Damage_Fire,
		TAG_Damage_Ice,
		TAG_Damage_Lightning,
		TAG_Damage_Poison
	};
	for (const FGameplayTag& Element : ElementTags)
	{
		Raw += Spec.GetSetByCallerMagnitude(Element, false, 0.0f);
	}
	Raw *= FMath::FRandRange(0.85f, 1.15f);

	if (Raw <= 0.0f || !TargetCombat)
	{
		return;
	}

	// Step 2: Block - the target's shield chance negates a portion before mitigation.
	float BlockFactor = 1.0f;
	const float BlockChance = FMath::Clamp(TargetCombat->GetBlockChance(), 0.0f, 1.0f);
	if (BlockChance > 0.0f)
	{
		BlockFactor = 1.0f - (BlockChance * BlockDamageReduction);
	}
	Raw *= BlockFactor;

	// Step 3+4: Mitigation + type chart - each element is reduced by DEF/(DEF+K) or
	// RES/(RES+K), then multiplied by the element chart (1.5/1.0/0.5) vs the target's
	// affinity (Element.* tag; no tag = neutral = 1.0). Sum the element contributions.
	const EOnsetDamageElement TargetElement = ResolveTargetElement(TargetASC);
	float Mitigated = 0.0f;
	for (const FGameplayTag& Element : ElementTags)
	{
		const float ElementValue = Spec.GetSetByCallerMagnitude(Element, false, 0.0f);
		if (ElementValue <= 0.0f)
		{
			continue;
		}

		const FGameplayAttribute MitigationAttribute =
			TargetCombat->GetMitigationAttributeForDamageTag(Element);
		float MitigationStat = 0.0f;
		if (MitigationAttribute.IsValid())
		{
			MitigationStat = FMath::Max(0.0f, MitigationAttribute.GetNumericValueChecked(TargetCombat));
		}

		const float K = (Element == TAG_Damage_Physical) ? K_Defense : K_Elemental;
		const float Mitigation = MitigationStat / (MitigationStat + K);

		const float TypeMultiplier = UOnsetAbilityLibrary::GetElementAffinityMultiplier(
			UOnsetAbilityLibrary::GetElementFromDamageTag(Element), TargetElement);

		Mitigated += ElementValue * (1.0f - Mitigation) * TypeMultiplier;
	}

	// Step 5: Crit - LUK curves from the source. 5% base, +LUK/(LUK+200), capped 70%;
	// multiplier 1.5x rising toward 4.0x with LUK.
	float FinalDamage = Mitigated;
	if (SourceCombat)
	{
		const float Luck = FMath::Max(0.0f, SourceCombat->GetLuck());
		float CritChance = BaseCritChance + Luck / (Luck + K_Crit);
		const float CritMultiplier = FMath::Clamp(
			BaseCritMultiplier + (Luck / (Luck + K_CritMultiplier)) * (MaxCritMultiplier - BaseCritMultiplier),
			BaseCritMultiplier, MaxCritMultiplier);

		// Ranged mastery: +10% crit while wielding a Bow (combat-formulas §11).
		if (const AOnsetBaseCharacter* SourceActor = Cast<AOnsetBaseCharacter>(SourceASC ? SourceASC->GetOwnerActor() : nullptr))
		{
			if (SourceActor->GetCharacterClass() == EOnsetCharacterClass::Ranged)
			{
				const FOnsetEquipmentDefinition* Weapon = SourceActor->GetEquippedItem(EOnsetEquipmentSlot::Weapon);
				if (Weapon && Weapon->Archetype == EOnsetWeaponArchetype::Bow)
				{
					CritChance += UOnsetEquipmentLibrary::GetBowMasteryCritBonus();
				}
			}
		}

		if (FMath::FRand() <= FMath::Clamp(CritChance, 0.0f, MaxCritChance))
		{
			FinalDamage *= CritMultiplier;
		}
	}

	// Step 6: Buffs/Debuffs - additive outgoing (caster) and incoming (target) damage
	// mods from GE modifiers on OutgoingDamageMod/IncomingDamageMod, plus the prestige
	// outgoing multiplier (1 + r)^N.
	const float OutgoingMod = SourceCombat ? SourceCombat->GetOutgoingDamageMod() : 0.0f;
	const float IncomingMod = TargetCombat->GetIncomingDamageMod();
	const float Prestige = SourceCombat ? FMath::Max(0.0f, SourceCombat->GetPrestigeMultiplier()) : 1.0f;
	FinalDamage *= (1.0f + OutgoingMod) * (1.0f + IncomingMod) * Prestige;

	if (FinalDamage <= 0.0f)
	{
		return;
	}

	const UOnsetAttributeSet* TargetAttributes = TargetASC->GetSet<UOnsetAttributeSet>();
	if (!TargetAttributes)
	{
		return;
	}

	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
		TargetAttributes->GetHealthAttribute(),
		EGameplayModOp::Additive,
		-FinalDamage));
}

EOnsetDamageElement UOnsetDamageExecution::ResolveTargetElement(UAbilitySystemComponent* TargetASC) const
{
	// Target affinity is expressed as an owned Element.* tag (e.g. Element.Ice on an
	// ice golem). No tag (or an explicit Element.Neutral) resolves to Physical, which
	// the chart treats as 1.0 from every source element.
	if (TargetASC->HasMatchingGameplayTag(TAG_Element_Fire))			return EOnsetDamageElement::Fire;
	if (TargetASC->HasMatchingGameplayTag(TAG_Element_Ice))				return EOnsetDamageElement::Ice;
	if (TargetASC->HasMatchingGameplayTag(TAG_Element_Lightning))		return EOnsetDamageElement::Lightning;
	if (TargetASC->HasMatchingGameplayTag(TAG_Element_Poison))			return EOnsetDamageElement::Poison;
	return EOnsetDamageElement::Physical;
}