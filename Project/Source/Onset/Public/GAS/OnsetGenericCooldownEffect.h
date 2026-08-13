// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "OnsetGenericCooldownEffect.generated.h"

/**
 * GE_GenericCooldown template - shared cooldown GE for data-driven abilities.
 *
 * Has a duration (SetByCaller "Duration") and grants no tags by default; the
 * per-ability cooldown tag (Cooldown.<RowName>) is added dynamically at apply time.
 */
UCLASS()
class ONSET_API UOnsetGenericCooldownEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UOnsetGenericCooldownEffect();
};
