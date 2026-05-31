#include "AI/OnsetStateTreeContextTask.h"

#include "AI/OnsetStateTreeContext.h"
#include "AI/OnsetAIController.h"
#include "Enemy/OnsetEnemy.h"
#include "StateTreeExecutionContext.h"
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
	
	return EStateTreeRunStatus::Running;
}
	
EStateTreeRunStatus FOnsetStateTreeContextTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	return EnterState(Context, FStateTreeTransitionResult());
}          