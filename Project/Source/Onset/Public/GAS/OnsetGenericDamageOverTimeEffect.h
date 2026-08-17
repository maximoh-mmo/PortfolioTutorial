// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "OnsetGenericDamageOverTimeEffect.generated.h"

/**
 * GE_GenericDamageOT template - the damage-over-time effect.
 *
 * HasDuration + Period. Each tick runs UOnsetDamageExecution, so DOT goes through
 * the same damage pipeline as instant damage (invulnerability gate, physical/magical
 * split, future mitigation stages). The SetByCaller "Duration" and per-tick damage
 * magnitudes (Damage.<Element>) are set on the spec at apply time.
 */
UCLASS()
class ONSET_API UOnsetGenericDamageOverTimeEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UOnsetGenericDamageOverTimeEffect();
};