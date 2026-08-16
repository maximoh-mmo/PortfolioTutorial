// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "OnsetGenericInvulnerableEffect.generated.h"

/**
 * GE_GenericInvulnerable template - the damage immunity buff.
 *
 * Has a duration (SetByCaller "Duration") and grants State.Invulnerable for that
 * window, which negates all incoming damage (see UOnsetDamageExecution).
 * Re-casting refreshes the window instead of stacking.
 */
UCLASS()
class ONSET_API UOnsetGenericInvulnerableEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UOnsetGenericInvulnerableEffect();
};