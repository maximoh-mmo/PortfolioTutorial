// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTree/Tasks/OnsetStateTreeTask.h"
#include "EnemyLostTargetTask.generated.h"

/** Runtime data for the enemy lost-target task. */
USTRUCT()
struct FOnsetStateTreeLostTargetInstanceData
{
	GENERATED_BODY()

	/** Minimum time to remain in the lost-target state. */
	UPROPERTY(EditAnywhere, Category = "LostTarget")
	float MinDuration = 2.0f;

	/** Maximum time to remain in the lost-target state. */
	UPROPERTY(EditAnywhere, Category = "LostTarget")
	float MaxDuration = 4.0f;

	/** Countdown timer before transitioning out. */
	float RemainingTime = 0.0f;

};

USTRUCT()
struct FEnemyLostTargetTask : public FOnsetStateTreeTask
{
	GENERATED_BODY()

	using FInstanceDataType = FOnsetStateTreeLostTargetInstanceData;
	
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