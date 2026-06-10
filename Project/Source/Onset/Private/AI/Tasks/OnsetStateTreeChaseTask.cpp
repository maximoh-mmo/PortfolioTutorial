#include "AI/Tasks/OnsetStateTreeChaseTask.h"

#include "StateTreeExecutionContext.h"
#include "AI/OnsetAIController.h"

EStateTreeRunStatus FOnsetStateTreeChaseTask::EnterState(FStateTreeExecutionContext& Context,
                                                         const FStateTreeTransitionResult& Transition) const
{
	AOnsetAIController* AIController = GetController(Context);
	if (!AIController) return EStateTreeRunStatus::Failed;
	AActor* TargetActor = GetTarget(Context);
	// No target to chase, but not a failure of the task itself.
	if (!TargetActor) return EStateTreeRunStatus::Succeeded; 
	
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.ChaseStartTime = AIController->GetWorld()->GetTimeSeconds();
	AIController->MoveToActor(TargetActor, InstanceData.AcceptanceRadius);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FOnsetStateTreeChaseTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	AOnsetAIController* AIController = GetController(Context);
	if (!AIController) return EStateTreeRunStatus::Failed;
	
	UPathFollowingComponent* PathFollowingComponent = GetPathFollowingComponent(Context);
	if (!PathFollowingComponent) return EStateTreeRunStatus::Failed;
	if (HasMoveCompleted(Context)) return EStateTreeRunStatus::Succeeded;

	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	float Elapsed = AIController->GetWorld()->GetTimeSeconds() - InstanceData.ChaseStartTime;
	if (Elapsed > InstanceData.MaxChaseDuration)
		return EStateTreeRunStatus::Failed;

	return EStateTreeRunStatus::Running;
}

void FOnsetStateTreeChaseTask::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	AOnsetAIController* AIController = GetController(Context);
	if (!AIController) return;
	AIController->StopMovement();
}
