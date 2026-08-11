// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "OnsetCooldownSlowEffect.generated.h"

/**
 * GE_GenericSlow template — the Slow debuff.
 *
 * Has a duration and multiplies the target's CooldownMultiplier by the
 * SetByCaller magnitude "CooldownRateMod". A magnitude above 1 raises the
 * multiplier, so the target's subsequent cooldowns last proportionally longer
 * (base cooldown x CooldownMultiplier). Duration is also a SetByCaller
 * ("Duration" tag) so callers pick the window per application.
 */
UCLASS()
class ONSET_API UOnsetCooldownSlowEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UOnsetCooldownSlowEffect();
};
