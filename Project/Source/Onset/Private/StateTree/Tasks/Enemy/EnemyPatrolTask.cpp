#include "StateTree/Tasks/Enemy/EnemyPatrolTask.h"

#include "NavigationSystem.h"
#include "StateTreeExecutionContext.h"
#include "AI/OnsetAIController.h"
#include "Core/OnsetBaseCharacter.h"
#include "Navigation/PathFollowingComponent.h"

EStateTreeRunStatus FEnemyPatrolTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Inst = Context.GetInstanceData(*this);
	Inst.bHasArrived = false;
	Inst.PauseTimer = 0.0f;

	AOnsetAIController* AIController = GetController(Context);
	if (!AIController) return EStateTreeRunStatus::Failed;
	AOnsetBaseCharacter* Self = GetSelfPawn<AOnsetBaseCharacter>(Context);
	if (!Self) return EStateTreeRunStatus::Failed;

	if (FMath::FRand() < Inst.RoamChance)
	{
		Inst.bIsRoaming = true;
		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(AIController->GetWorld());
		if (!NavSys) return EStateTreeRunStatus::Failed;

		FNavLocation RandomPoint;
		if (NavSys->GetRandomReachablePointInRadius(Self->HomeTransform.GetLocation(), Inst.RoamRadius, RandomPoint))
		{
			Inst.Destination = RandomPoint.Location;
			AIController->MoveToLocation(Inst.Destination, Inst.AcceptanceRadius);
			return EStateTreeRunStatus::Running;
		}
		return EStateTreeRunStatus::Failed;
	}

	Inst.bIsRoaming = false;
	Inst.RemainingTime = FMath::FRandRange(Inst.MinIdleDuration, Inst.MaxIdleDuration);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyPatrolTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Inst = Context.GetInstanceData(*this);

	if (!Inst.bIsRoaming)
	{
		Inst.RemainingTime -= DeltaTime;
		return Inst.RemainingTime <= 0.0f ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
	}

	if (Inst.bHasArrived)
	{
		Inst.PauseTimer -= DeltaTime;
		if (Inst.PauseTimer <= 0.0f)
			return EStateTreeRunStatus::Succeeded;
		return EStateTreeRunStatus::Running;
	}

	UPathFollowingComponent* PFComponent = GetPathFollowingComponent(Context);
	if (!PFComponent) return EStateTreeRunStatus::Failed;

	const EPathFollowingStatus::Type MoveStatus = PFComponent->GetStatus();
	if (MoveStatus == EPathFollowingStatus::Type::Idle)
	{
		if (HasMoveCompleted(Context))
		{
			Inst.bHasArrived = true;
			Inst.PauseTimer = Inst.PauseOnArrival;
		}
		else
		{
			return EStateTreeRunStatus::Succeeded;
		}
	}

	return EStateTreeRunStatus::Running;
}

void FEnemyPatrolTask::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	AOnsetAIController* AIController = GetController(Context);
	if (AIController) AIController->StopMovement();
}
