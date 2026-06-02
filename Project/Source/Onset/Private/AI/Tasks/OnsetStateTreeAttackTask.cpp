#include "AI/Tasks/OnsetStateTreeAttackTask.h"

EStateTreeRunStatus FOnsetStateTreeAttackTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	AOnsetAIController* AIController = GetController(Context);
	if (!AIController) return EStateTreeRunStatus::Failed;
	AActor* Target = GetTarget(Context);
	if (!Target) return EStateTreeRunStatus::Failed;
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.RemainingCooldown = InstanceData.CooldownDuration; 	
	AIController->StopMovement();
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FOnsetStateTreeAttackTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.RemainingCooldown -= DeltaTime;
	if (InstanceData.RemainingCooldown <= 0.0f)
	{
		return EStateTreeRunStatus::Succeeded; // Attack completed
	}
	return EStateTreeRunStatus::Running; // Still cooling down
}
