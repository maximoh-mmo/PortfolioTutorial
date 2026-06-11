#include "StateTree/Tasks/Enemy/ChaseTask.h"

#include "StateTreeExecutionContext.h"
#include "Enemy/OnsetAIController.h"
#include "Engine/World.h"

EStateTreeRunStatus FChaseTask::EnterState(FStateTreeExecutionContext& Context,
                                                         const FStateTreeTransitionResult& Transition) const
{
	AOnsetAIController* AIController = GetController(Context);
	if (!AIController) return EStateTreeRunStatus::Failed;
	AActor* TargetActor = GetTarget(Context);
	// No target to chase, but not a failure of the task itself.
	if (!TargetActor) return EStateTreeRunStatus::Succeeded; 
	
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.ChaseStartTime = AIController->GetWorld()->GetTimeSeconds();
	const FVector TargetLoc = TargetActor->GetActorLocation();
	const FVector SelfLoc = AIController->GetPawn()->GetActorLocation();
	const FVector ApproachDir = (SelfLoc - TargetLoc).GetSafeNormal2D();
	const FVector Right = FVector::CrossProduct(ApproachDir, FVector::UpVector).GetSafeNormal();
	const float Spread = FMath::RandRange(InstanceData.SpreadRadius * 0.5f, InstanceData.SpreadRadius);
	const float LateralT = FMath::RandRange(-1.0f, 1.0f);
	InstanceData.OffsetLocation = TargetLoc + Right * LateralT * Spread;      
	AIController->MoveToLocation(InstanceData.OffsetLocation, InstanceData.AcceptanceRadius); 
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FChaseTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
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

void FChaseTask::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	AOnsetAIController* AIController = GetController(Context);
	if (!AIController) return;
	AIController->StopMovement();
}
