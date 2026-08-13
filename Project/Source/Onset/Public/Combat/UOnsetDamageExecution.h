// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "UOnsetDamageExecution.generated.h"

/**
 * Central damage execution for the data-driven ability pipeline.
 *
 * Reads the SetByCaller magnitudes "Damage.Physical" and "Damage.Magical" from the
 * effect spec, applies the (v1 = pass-through) mitigation pipeline, and outputs a
 * negative additive modifier on the target's Health. The v1 formula is damage x 1.0
 * with an invulnerability gate (TAG_State_Invulnerable -> 0), so all damage negation
 * lives in one place. Future stages (variance, armor/resist, crit) slot into
 * Execute_Implementation without changing the SetByCaller seam.
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
};
