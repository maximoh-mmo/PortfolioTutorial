// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "StateTree/Tasks/OnsetStateTreeTask.h"
#include "PlayerAcquireTargetTask.generated.h"

/** Runtime data for the player acquire target task. */
USTRUCT()
struct FPlayerAcquireTargetTaskInstanceData
{
	GENERATED_BODY()

	/** Minimum time between target search evaluations. */
	UPROPERTY(EditAnywhere, Category = "Acquire")
	float SearchInterval = 0.5f;

	/** World time of the last target search. */
	float LastSearchTime = 0.0f;
};                                                                                                                                                                           
                                                                                                                                                                                  
USTRUCT()       
struct FPlayerAcquireTargetTask : public FOnsetStateTreeTask
{
	GENERATED_BODY()
	
	using FInstanceDataType = FPlayerAcquireTargetTaskInstanceData;
	
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct(); 
	}
	
	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(
		FStateTreeExecutionContext& Context, const float DeltaTime) const override;   
};
