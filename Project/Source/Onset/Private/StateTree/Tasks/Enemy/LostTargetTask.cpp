// Fill out your copyright notice in the Description page of Project Settings.

#include "StateTree/Tasks/Enemy/LostTargetTask.h"

#include "StateTreeExecutionContext.h"
#include "Enemy/OnsetAIController.h"

EStateTreeRunStatus FLostTargetTask::EnterState(FStateTreeExecutionContext& Context,
                                                              const FStateTreeTransitionResult& Transition) const
{
	AOnsetAIController* AIController = GetController(Context);
	if (!AIController) return EStateTreeRunStatus::Failed;
	AIController->ClearFocus(EAIFocusPriority::Gameplay);
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.RemainingTime = FMath::FRandRange(InstanceData.MinDuration, InstanceData.MaxDuration);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FLostTargetTask::Tick(FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.RemainingTime -= DeltaTime;
	if (InstanceData.RemainingTime <= 0.0f)
	{
		return EStateTreeRunStatus::Succeeded;
	}
	return EStateTreeRunStatus::Running;
}
