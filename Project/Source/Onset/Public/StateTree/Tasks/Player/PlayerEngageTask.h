// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "StateTree/Tasks/OnsetStateTreeTask.h"
#include "PlayerEngageTask.generated.h"

/** Runtime data for the player engage task. */
USTRUCT()
struct FPlayerEngageTaskInstanceData
{
	GENERATED_BODY()

	/** Distance at which the NPC stops moving toward the target and starts attacking. */
	UPROPERTY(EditAnywhere, Category = "Engage")
	float AcceptanceRadius = 50.0f;

	/** Maximum time the engage task runs before timing out. */
	UPROPERTY(EditAnywhere, Category = "Engage")
	float MaxEngageDuration = 8.0f;

	/** Radius for AoE overlap checks when selecting multi-target abilities. */
	UPROPERTY(EditAnywhere, Category = "Engage")
	float AoEOverlapRadius = 400.0f;

	/** Minimum interval between ability activation ticks. */
	UPROPERTY(EditAnywhere, Category = "Engage")
	float AttackTickInterval = 0.25f;

	/** World time when Engage state entered. */
	float EngageStartTime = 0.0f;
	/** World time of the last attack tick. */
	float LastAttackTick = 0.0f;
};

USTRUCT()
struct FPlayerEngageTask : public FOnsetStateTreeTask
{
	GENERATED_BODY()
	
	using FInstanceDataType = FPlayerEngageTaskInstanceData;
	
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}
	
	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	
	virtual EStateTreeRunStatus Tick(
		FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	
	virtual void ExitState(
		FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
