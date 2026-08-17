// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "OnsetGenericFreezeEffect.generated.h"

/**
 * GE_GenericFreeze template - the Ice-element hard crowd control.
 *
 * Mirrors GE_GenericStun: has a duration (SetByCaller "Duration") and grants
 * State.Frozen for that window, which blocks ability activation. Re-casting
 * refreshes the freeze instead of stacking.
 */
UCLASS()
class ONSET_API UOnsetGenericFreezeEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UOnsetGenericFreezeEffect();
};