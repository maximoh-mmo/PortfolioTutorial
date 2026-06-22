// Fill out your copyright notice in the Description page of Project Settings.

#include "StateTree/Tasks/Enemy/EnemyAgroTask.h"
#include "StateTreeExecutionContext.h"
#include "AI/OnsetAIController.h"

EStateTreeRunStatus FEnemyAgroTask::EnterState(FStateTreeExecutionContext& Context,
                                                        const FStateTreeTransitionResult& Transition) const
{
	AOnsetAIController* AIController = GetController(Context);
	if (!AIController) return EStateTreeRunStatus::Failed;
	AActor* Target = GetTarget(Context);
	if (!Target) return EStateTreeRunStatus::Succeeded;
	AIController->SetFocus(Target);	
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyAgroTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.TimeSpent += DeltaTime;
	
	AOnsetAIController* AIController = GetController(Context);
	if (!AIController) return EStateTreeRunStatus::Failed;
	AActor* Target = GetTarget(Context);
	// No target means we're done agroing, so succeed to transition out.
	if (!Target) return EStateTreeRunStatus::Succeeded;
	
	FVector ToTarget = Target->GetActorLocation() - AIController->GetPawn()->GetActorLocation();
	ToTarget.Z = 0.0f;
	ToTarget.Normalize();
	FVector Forward = AIController->GetPawn()->GetActorForwardVector();
	float FacingAngle = FMath::Acos(FVector::DotProduct(Forward, ToTarget)) * (180.0f / PI);
	
	if (FacingAngle <= InstanceData.FacingThreshold && InstanceData.TimeSpent >= InstanceData.MinDuration)
	{
		return EStateTreeRunStatus::Succeeded;
	}
	return EStateTreeRunStatus::Running;
}
