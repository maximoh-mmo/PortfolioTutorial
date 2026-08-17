// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "OnsetCCDiminishingComponent.generated.h"

/**
 * CC diminishing-returns tracker (combat-formulas: 100% → 50% → 25% → immune).
 *
 * Each character owns one of these. Hard-CC durations (Stun, Freeze) are run through
 * GetDiminishedDuration() before the GE is applied: consecutive applications of the
 * same CC tag within CCDiminishingWindow get 100% / 50% / 25% / 0% (immune) of their
 * base duration, so a target can't be chain-locked. A returning 0 means "immune" —
 * callers must skip applying the effect.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ONSET_API UOnsetCCDiminishingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * Effective duration for BaseDuration of CCType, registering this application.
	 * Returns 0 when the target is CC-immune (4+ in-window applications).
	 */
	float GetDiminishedDuration(FGameplayTag CCType, float BaseDuration);

	/** Resets the DR tracker (e.g. on respawn). */
	void ResetDiminishingReturns();

private:
	/** Applications within this window reduce the next duration; longer gaps reset the chain. */
	UPROPERTY(EditDefaultsOnly, Category = "CC")
	float CCDiminishingWindow = 6.0f;

	/** Last application timestamp per CC tag. */
	UPROPERTY()
	TMap<FGameplayTag, double> LastCCApplicationTimes;

	/** Consecutive in-window application count per CC tag. */
	UPROPERTY()
	TMap<FGameplayTag, int32> CCStacks;
};