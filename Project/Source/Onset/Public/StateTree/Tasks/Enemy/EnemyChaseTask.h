// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTree/Tasks/OnsetStateTreeTask.h"
#include "EnemyChaseTask.generated.h"

/** Runtime data for the enemy chase task. */
USTRUCT()
struct FOnsetStateTreeChaseTaskInstanceData
{
	GENERATED_BODY()

	/** Distance threshold for arriving at the target. */
	UPROPERTY(EditAnywhere, Category = "Chase")
	float AcceptanceRadius = 50.0f;

	/** Maximum time to chase before giving up. */
	UPROPERTY(EditAnywhere, Category = "Chase")
	float MaxChaseDuration = 3.0f;

	/** Lateral spread radius applied to offset position during formation. */
	UPROPERTY(EditAnywhere, Category = "Chase")
	float SpreadRadius = 280.0f;

	/** Lateral offset from the target's location for formation positioning. */
	FVector OffsetLocation = FVector::ZeroVector;
	/** World time when the chase started. */
	float ChaseStartTime = 0.0f;
};

USTRUCT()
struct FEnemyChaseTask : public FOnsetStateTreeTask
{
	GENERATED_BODY()

	using FInstanceDataType = FOnsetStateTreeChaseTaskInstanceData;
	
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
