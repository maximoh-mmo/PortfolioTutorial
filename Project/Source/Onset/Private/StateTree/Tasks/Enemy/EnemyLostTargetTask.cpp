// Fill out your copyright notice in the Description page of Project Settings.

#include "StateTree/Tasks/Enemy/EnemyLostTargetTask.h"

#include "StateTreeExecutionContext.h"
#include "AI/OnsetAIController.h"
#include "Enemy/OnsetEnemy.h"
#include "GAS/OnsetAttributeSet.h"
#include "Navigation/PathFollowingComponent.h"
#include "Subsystem/OnsetThreatSubsystem.h"

EStateTreeRunStatus FEnemyLostTargetTask::EnterState(FStateTreeExecutionContext& Context,
                                                               const FStateTreeTransitionResult& Transition) const
{
	AOnsetAIController* AIController = GetController(Context);
	if (!AIController) return EStateTreeRunStatus::Failed;
	if (!AIController->HasAuthority()) return EStateTreeRunStatus::Failed;
	AIController->ClearFocus(EAIFocusPriority::Gameplay);

	AOnsetEnemy* SelfEnemy = GetSelfPawn<AOnsetEnemy>(Context);
	if (UOnsetThreatSubsystem* Subsystem = GetThreatSubsystem(Context))
	{
		if (SelfEnemy)
		{
			Subsystem->UnregisterEngaged(SelfEnemy);
		}
	}

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	// If returning home after leashing, evade back to HomeTransform and reset health
	if (AIController->bIsReturningHome && SelfEnemy)
	{
		if (SelfEnemy->AttributeSet)
		{
			SelfEnemy->AttributeSet->SetHealth(SelfEnemy->AttributeSet->GetMaxHealth());
		}
		AIController->MoveToLocation(SelfEnemy->HomeTransform.GetLocation(), 75.0f);
		InstanceData.RemainingTime = 10.0f; // Safety timeout for return-to-home walk
		return EStateTreeRunStatus::Running;
	}

	InstanceData.RemainingTime = FMath::FRandRange(InstanceData.MinDuration, InstanceData.MaxDuration);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyLostTargetTask::Tick(FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	AOnsetAIController* AIController = GetController(Context);
	if (!AIController) return EStateTreeRunStatus::Failed;

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.RemainingTime -= DeltaTime;

	if (AIController->bIsReturningHome)
	{
		AOnsetEnemy* SelfEnemy = GetSelfPawn<AOnsetEnemy>(Context);
		const bool bNearHome = SelfEnemy &&
			FVector::DistSquared2D(SelfEnemy->GetActorLocation(), SelfEnemy->HomeTransform.GetLocation()) <= FMath::Square(100.0f);
		const UPathFollowingComponent* PF = GetPathFollowingComponent(Context);
		const bool bMoveDone = HasMoveCompleted(Context) || (PF && PF->GetStatus() == EPathFollowingStatus::Idle);

		if (bNearHome || bMoveDone || InstanceData.RemainingTime <= 0.0f)
		{
			AIController->bIsReturningHome = false;
			return EStateTreeRunStatus::Succeeded;
		}
		return EStateTreeRunStatus::Running;
	}

	// If a target was re-acquired while pausing, finish immediately so StateTree can re-engage
	if (GetTarget(Context) != nullptr)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	if (InstanceData.RemainingTime <= 0.0f)
	{
		return EStateTreeRunStatus::Succeeded;
	}
	return EStateTreeRunStatus::Running;
}

void FEnemyLostTargetTask::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	if (AOnsetAIController* AIController = GetController(Context))
	{
		AIController->bIsReturningHome = false;
		AIController->StopMovement();
	}
}
