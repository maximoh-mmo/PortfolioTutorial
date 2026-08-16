// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "OnsetGenericStunEffect.generated.h"

/**
 * GE_GenericStun template - the crowd-control debuff.
 *
 * Has a duration (SetByCaller "Duration") and grants State.Stunned for that window,
 * which blocks ability activation (see UOnsetGA_Generic::ValidateActivation).
 * Re-casting refreshes the stun instead of stacking.
 */
UCLASS()
class ONSET_API UOnsetGenericStunEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UOnsetGenericStunEffect();
};