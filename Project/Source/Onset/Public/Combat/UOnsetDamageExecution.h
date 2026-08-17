// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "Data/OnsetAbilityTypes.h"
#include "UOnsetDamageExecution.generated.h"

class UAbilitySystemComponent;

/**
 * Central damage execution for the data-driven ability pipeline.
 *
 * Reads the SetByCaller magnitudes "Damage.Physical/Fire/Ice/Lightning/Poison" from the
 * effect spec, applies the v2 mitigation pipeline, and outputs a negative additive
 * modifier on the target's Health. The v2 pipeline (see Docs/combat-formulas.md §5) is:
 *
 *   Raw → Variance (±15%) → Block (shield, before mitigation) → Mitigation
 *   (DEF/RES per element, K per zone tier) → Type multiplier → Crit (LUK curves)
 *   → Buffs/Debuffs → Health
 *
 * with the invulnerability gate (TAG_State_Invulnerable -> 0) at the top, so all damage
 * negation lives in one place. Later phases refine individual stages (type chart in
 * Phase 4, LUK crit curves + buff aggregation in Phase 5) without changing the
 * SetByCaller seam or the Health output.
 */
UCLASS()
class ONSET_API UOnsetDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UOnsetDamageExecution();

protected:
	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

private:
	/** Target's element affinity from an owned Element.* tag; Physical (= neutral) fallback. */
	EOnsetDamageElement ResolveTargetElement(UAbilitySystemComponent* TargetASC) const;
};
