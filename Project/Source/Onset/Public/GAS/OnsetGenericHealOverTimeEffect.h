// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "OnsetGenericHealOverTimeEffect.generated.h"

/**
 * GE_GenericHealOT template - the heal-over-time effect.
 *
 * HasDuration + Period. Additively applies the SetByCaller magnitude "HealAmount" to
 * the target's Health on every tick; the attribute set clamps Health to [0, MaxHealth].
 * The SetByCaller "Duration" is set on the spec at apply time.
 */
UCLASS()
class ONSET_API UOnsetGenericHealOverTimeEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UOnsetGenericHealOverTimeEffect();
};