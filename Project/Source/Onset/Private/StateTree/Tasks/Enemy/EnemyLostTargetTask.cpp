// Fill out your copyright notice in the Description page of Project Settings.

#include "StateTree/Tasks/Enemy/EnemyLostTargetTask.h"

#include "StateTreeExecutionContext.h"
#include "AI/OnsetAIController.h"
#include "Enemy/OnsetEnemy.h"
#include "Subsystem/OnsetThreatSubsystem.h"

EStateTreeRunStatus FEnemyLostTargetTask::EnterState(FStateTreeExecutionContext& Context,
                                                               const FStateTreeTransitionResult& Transition) const
{
	AOnsetAIController* AIController = GetController(Context);
	if (!AIController) return EStateTreeRunStatus::Failed;
	if (!AIController->HasAuthority()) return EStateTreeRunStatus::Failed;
	AIController->ClearFocus(EAIFocusPriority::Gameplay);

	if (UOnsetThreatSubsystem* Subsystem = GetThreatSubsystem(Context))
	{
		if (AOnsetEnemy* SelfEnemy = GetSelfPawn<AOnsetEnemy>(Context))
			Subsystem->UnregisterEngaged(SelfEnemy);
	}

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.RemainingTime = FMath::FRandRange(InstanceData.MinDuration, InstanceData.MaxDuration);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyLostTargetTask::Tick(FStateTreeExecutionContext& Context,
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
