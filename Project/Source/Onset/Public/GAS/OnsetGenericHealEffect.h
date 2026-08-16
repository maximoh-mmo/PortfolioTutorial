// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "OnsetGenericHealEffect.generated.h"

/**
 * GE_GenericHeal template - the instant heal.
 *
 * Additively applies the SetByCaller magnitude "HealAmount" to the target's Health.
 * The attribute set clamps Health to [0, MaxHealth], so overheal is discarded.
 */
UCLASS()
class ONSET_API UOnsetGenericHealEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UOnsetGenericHealEffect();
};