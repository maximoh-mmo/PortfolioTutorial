// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/Tasks/OnsetStateTreeLostTargetTask.h"

#include "StateTreeExecutionContext.h"
#include "AI/OnsetAIController.h"

EStateTreeRunStatus FOnsetStateTreeLostTargetTask::EnterState(FStateTreeExecutionContext& Context,
                                                              const FStateTreeTransitionResult& Transition) const
{
	AOnsetAIController* AIController = GetController(Context);
	if (!AIController) return EStateTreeRunStatus::Failed;
	AIController->ClearFocus(EAIFocusPriority::Gameplay);
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.RemainingTime = FMath::FRandRange(InstanceData.MinDuration, InstanceData.MaxDuration);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FOnsetStateTreeLostTargetTask::Tick(FStateTreeExecutionContext& Context,
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
