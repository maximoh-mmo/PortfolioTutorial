// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "OnsetGenericSnareEffect.generated.h"

/**
 * GE_GenericSnare template - the movement-speed debuff.
 *
 * Has a duration and multiply-compounds the target's MovementSpeed by the SetByCaller
 * magnitude "MoveSpeedMod" (values < 1 slow the target). Duration is a SetByCaller
 * ("Duration") so callers pick the window per application.
 */
UCLASS()
class ONSET_API UOnsetGenericSnareEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UOnsetGenericSnareEffect();
};
