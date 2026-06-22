// Fill out your copyright notice in the Description page of Project Settings.


#include "StateTree/Tasks/Enemy/EnemyIdleTask.h"

#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FEnemyIdleTask::EnterState(FStateTreeExecutionContext& Context,
                                                        const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	InstanceData.RemainingDuration = FMath::RandRange(InstanceData.MinDuration, InstanceData.MaxDuration);
	
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyIdleTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.RemainingDuration -= DeltaTime;
	return InstanceData.RemainingDuration <= 0.0f 
		? EStateTreeRunStatus::Succeeded 
		: EStateTreeRunStatus::Running;
}
