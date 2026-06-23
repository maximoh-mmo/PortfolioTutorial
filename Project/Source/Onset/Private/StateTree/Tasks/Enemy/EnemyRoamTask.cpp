// Fill out your copyright notice in the Description page of Project Settings.


#include "StateTree/Tasks/Enemy/EnemyRoamTask.h"

#include "NavigationSystem.h"
#include "StateTreeExecutionContext.h"
#include "AI/OnsetAIController.h"
#include "Enemy/OnsetEnemy.h"
#include "Navigation/PathFollowingComponent.h"

EStateTreeRunStatus FEnemyRoamTask::EnterState(FStateTreeExecutionContext& Context,
                                                        const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.bHasArrived = false;
	InstanceData.PauseTimer = 0.0f;
	
	AOnsetAIController* AIController = GetController(Context);
	if (!AIController)  return EStateTreeRunStatus::Failed;
	AOnsetBaseCharacter* Self = GetSelfPawn<AOnsetBaseCharacter>(Context);
	if (!Self) return EStateTreeRunStatus::Failed;
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(AIController->GetWorld());               
	if (!NavSys) return EStateTreeRunStatus::Failed;
	
	FVector Home = Self->HomeTransform.GetLocation();
	FNavLocation RandomPoint;
	if (NavSys->GetRandomReachablePointInRadius(Home, InstanceData.RoamRadius, RandomPoint))
	{
		InstanceData.Destination = RandomPoint.Location;
		AIController->MoveToLocation(InstanceData.Destination, InstanceData.AcceptanceRadius);
		return EStateTreeRunStatus::Running;
	}
	return EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FEnemyRoamTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.bHasArrived)
	{
		InstanceData.PauseTimer -= DeltaTime;
		if (InstanceData.PauseTimer <= 0.0f)
		{
			return EStateTreeRunStatus::Succeeded;
		}
		return EStateTreeRunStatus::Running;
	}
	
	UPathFollowingComponent* PFComponent = GetPathFollowingComponent(Context);
	if (!PFComponent) return EStateTreeRunStatus::Failed;
	EPathFollowingStatus::Type MoveStatus  = PFComponent->GetStatus();
	if (MoveStatus == EPathFollowingStatus::Type::Idle)
	{
		if (HasMoveCompleted(Context))
		{
			InstanceData.bHasArrived = true;
			InstanceData.PauseTimer = InstanceData.PauseOnArrival;
		}
		else
		{
			return EStateTreeRunStatus::Succeeded;
		}
	}
	else if (MoveStatus == EPathFollowingStatus::Type::Moving)
	{
		return EStateTreeRunStatus::Running;
	}
	return EStateTreeRunStatus::Running;
}