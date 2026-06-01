// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/OnsetStateTreeContextTask.h"
#include "StateTreeExecutionContext.h"
#include "AI/OnsetStateTreeContext.h"
#include "AI/OnsetAIController.h"
#include "Enemy/OnsetEnemy.h"
#include "Components/StateTreeAIComponent.h"
#include "Player/TargetingComponent.h"

EStateTreeRunStatus FOnsetStateTreeContextTask::EnterState(FStateTreeExecutionContext& Context,
                                                           const FStateTreeTransitionResult& TransitionResult) const
{
	UE_LOG(LogStateTree, Log, TEXT("Entering StateTree context task"));
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	const AOnsetAIController* AIController = Cast<AOnsetAIController>(Context.GetOwner());
	if (!AIController) return EStateTreeRunStatus::Failed;
	
	if (AIController->TargetingComponent)
	{
		InstanceData.Target = AIController->TargetingComponent->GetTarget();
	}

	const TArray<FName> Names = AIController->StateTreeComponent->GetActiveStateNames();
	UE_LOG(LogStateTree, Log, TEXT("State: %s"),                                                                             
		*FString::JoinBy(Names, TEXT(" > "), [](const FName& N) { return N.ToString(); }));
	return EStateTreeRunStatus::Running;
}
	
EStateTreeRunStatus FOnsetStateTreeContextTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	return EnterState(Context, FStateTreeTransitionResult());
}          