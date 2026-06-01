// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Tasks/OnsetStateTreeIdleTask.h"

#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FOnsetStateTreeIdleTask::EnterState(FStateTreeExecutionContext& Context,
                                                        const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	InstanceData.RemainingDuration = FMath::RandRange(InstanceData.MinDuration, InstanceData.MaxDuration);
	
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FOnsetStateTreeIdleTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.RemainingDuration -= DeltaTime;
	return InstanceData.RemainingDuration <= 0.0f 
		? EStateTreeRunStatus::Succeeded 
		: EStateTreeRunStatus::Running;
}
