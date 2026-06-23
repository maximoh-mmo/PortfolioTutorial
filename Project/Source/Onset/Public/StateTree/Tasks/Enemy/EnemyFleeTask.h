// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "StateTree/Tasks/OnsetStateTreeTask.h"
#include "EnemyFleeTask.generated.h"

/** Runtime data for the enemy flee task. */
USTRUCT()
struct FOnsetStateTreeFleeTaskInstanceData
{
	GENERATED_BODY()

	/** Distance to flee from the threat origin. */
	UPROPERTY(EditAnywhere, Category = "Flee")
	float FleeDistance = 1500.0f;

	/** Distance threshold for arriving at the flee destination. */
	UPROPERTY(EditAnywhere, Category = "Flee")
	float AcceptanceRadius = 50.0f;

	/** Minimum movement speed multiplier applied while fleeing. */
	UPROPERTY(EditAnywhere, Category = "Flee", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinSpeedMultiplier = 0.35f;

	/** Random angular variance when picking flee direction, in degrees. */
	UPROPERTY(EditAnywhere, Category = "Flee", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float FleeAngleVariance = 60.0f;

	/** Handle to the active speed-modifier gameplay effect. */
	FActiveGameplayEffectHandle SpeedEffectHandle;
	/** Destination point to flee toward. */
	FVector FleeDestination = FVector::ZeroVector;
};

USTRUCT()
struct FEnemyFleeTask : public FOnsetStateTreeTask
{
	GENERATED_BODY()

	using FInstanceDataType = FOnsetStateTreeFleeTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

	virtual EStateTreeRunStatus Tick(
		FStateTreeExecutionContext& Context,
		const float DeltaTime) const override;

	virtual void ExitState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;
};
