// Fill out your copyright notice in the Description page of Project Settings.


#include "StateTree/OnsetStateTreeContextTask.h"
#include "StateTreeExecutionContext.h"
#include "StateTree/OnsetStateTreeContext.h"
#include "Enemy/OnsetAIController.h"
#include "Enemy/OnsetEnemy.h"
#include "Components/StateTreeAIComponent.h"
#include "Player/TargetingComponent.h"

EStateTreeRunStatus FOnsetStateTreeContextTask::EnterState(FStateTreeExecutionContext& Context,
                                                           const FStateTreeTransitionResult& TransitionResult) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	const AOnsetAIController* AIController = Cast<AOnsetAIController>(Context.GetOwner());
	if (!AIController) return EStateTreeRunStatus::Failed;
	
	if (AIController->TargetingComponent)
	{
		InstanceData.Target = AIController->TargetingComponent->GetTarget();
	}

	const TArray<FName> Names = AIController->StateTreeComponent->GetActiveStateNames();
	return EStateTreeRunStatus::Running;
}
	
EStateTreeRunStatus FOnsetStateTreeContextTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	return EnterState(Context, FStateTreeTransitionResult());
}          