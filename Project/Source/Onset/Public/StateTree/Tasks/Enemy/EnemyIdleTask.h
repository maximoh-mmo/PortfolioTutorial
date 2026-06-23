// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTree/Tasks/OnsetStateTreeTask.h"
#include "EnemyIdleTask.generated.h"

/** Runtime data for the enemy idle task. */
USTRUCT()
struct FOnsetStateTreeIdleInstanceData
{
	GENERATED_BODY()

	/** Minimum time to remain idle. */
	UPROPERTY(EditAnywhere, Category="Idle")
	float MinDuration = 3.0f;

	/** Maximum time to remain idle. */
	UPROPERTY(EditAnywhere, Category="Idle")
	float MaxDuration = 10.0f;

	/** Countdown timer before transitioning out of idle. */
	float RemainingDuration = 0.0f;

};

USTRUCT()
struct FEnemyIdleTask : public FOnsetStateTreeTask
{
	GENERATED_BODY()

	using FInstanceDataType = FOnsetStateTreeIdleInstanceData;
	
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
};
