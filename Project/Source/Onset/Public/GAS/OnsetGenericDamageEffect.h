// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "OnsetGenericDamageEffect.generated.h"

/**
 * GE_GenericDamage template - the shared damage effect for the data-driven pipeline.
 *
 * Instant effect executed by UOnsetDamageExecution. The execution reads the SetByCaller
 * magnitudes "Damage.Physical" / "Damage.Magical" from the spec, runs the mitigation
 * pipeline, and outputs the final negative Health modifier. No damage value is ever baked
 * into this asset/class - callers always supply the magnitude at apply time.
 */
UCLASS()
class ONSET_API UOnsetGenericDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UOnsetGenericDamageEffect();
};
