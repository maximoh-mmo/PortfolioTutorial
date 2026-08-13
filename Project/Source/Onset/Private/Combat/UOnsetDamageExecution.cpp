// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/UOnsetDamageExecution.h"

#include "AbilitySystemComponent.h"
#include "GAS/OnsetAttributeSet.h"
#include "GAS/OnsetGameplayTags.h"

UOnsetDamageExecution::UOnsetDamageExecution()
{
	// No attributes captured yet. Future stages (variance, armor/resist) add
	// FGameplayEffectAttributeCaptureDefinition entries here; the SetByCaller seam
	// and the Health output never change.
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
	const float Physical = Spec.GetSetByCallerMagnitude(TAG_Damage_Physical, false, 0.0f);
	const float Magical = Spec.GetSetByCallerMagnitude(TAG_Damage_Magical, false, 0.0f);

	// ---- Damage pipeline (v1 = pass-through) ----
	// Future stages are inserted here in order: VarianceRoll -> Mitigation -> Type
	// multiplier -> Crit -> Buffs/Debuffs. Keeping them as discrete multiplicative
	// stages makes each independently tunable (see Docs/combat-formulas.md).
	float FinalDamage = (Physical + Magical) * 1.0f;
	// -------------------------------------------------

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
