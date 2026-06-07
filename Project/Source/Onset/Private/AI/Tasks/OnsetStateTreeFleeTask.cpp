#include "AI/Tasks/OnsetStateTreeFleeTask.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "StateTreeExecutionContext.h"
#include "AI/OnsetAIController.h"
#include "Combat/OnsetAttributeSet.h"
#include "Navigation/PathFollowingComponent.h"
#include "Player/OnsetBaseCharacter.h"

EStateTreeRunStatus FOnsetStateTreeFleeTask::EnterState(FStateTreeExecutionContext& Context,
                                                        const FStateTreeTransitionResult& Transition) const
{
	AOnsetAIController* AIController = GetController(Context);
	if (!AIController) return EStateTreeRunStatus::Failed;                                                      
	AIController->ClearFocus(EAIFocusPriority::Gameplay);
	AOnsetBaseCharacter* Self = GetSelfBaseCharacter(Context);
	if (!Self) return EStateTreeRunStatus::Failed;

	AActor* Target = GetTarget(Context);
	if (!Target) return EStateTreeRunStatus::Succeeded;

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(AIController->GetWorld());
	if (!NavSys) return EStateTreeRunStatus::Failed;

	FVector AwayDir = (Self->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal();
	if (AwayDir.IsNearlyZero())
	{
		AwayDir = -FVector::ForwardVector;
	}

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	float RandomAngle = FMath::RandRange(-InstanceData.FleeAngleVariance, InstanceData.FleeAngleVariance);
	FVector FleeDirection = AwayDir.RotateAngleAxis(RandomAngle, FVector::UpVector);

	FVector Desired = Self->HomeTransform.GetLocation() + FleeDirection * InstanceData.FleeDistance;

	FNavLocation Projected;
	if (NavSys->ProjectPointToNavigation(Desired, Projected, FVector(500.0f)))
	{
		InstanceData.FleeDestination = Projected.Location;
		InstanceData.CachedOriginalWalkSpeed = Self->GetCharacterMovement()->MaxWalkSpeed;
		AIController->MoveToLocation(InstanceData.FleeDestination, InstanceData.AcceptanceRadius);
		return EStateTreeRunStatus::Running;
	}

	return EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FOnsetStateTreeFleeTask::Tick(FStateTreeExecutionContext& Context,
                                                  const float DeltaTime) const
{
	AOnsetAIController* AIController = GetController(Context);
	if (!AIController) return EStateTreeRunStatus::Failed;
	if (!GetPathFollowingComponent(Context)) return EStateTreeRunStatus::Failed;

	if (AOnsetBaseCharacter* Self = GetSelfBaseCharacter(Context))
	{
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
		float HealthRatio = Self->AttributeSet->GetHealth() / Self->AttributeSet->GetMaxHealth();
		float SpeedMod = FMath::Lerp(InstanceData.MinSpeedMultiplier, 1.0f, HealthRatio);
		Self->GetCharacterMovement()->MaxWalkSpeed = InstanceData.CachedOriginalWalkSpeed * SpeedMod;
	}

	return HasMoveCompleted(Context) ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

void FOnsetStateTreeFleeTask::ExitState(FStateTreeExecutionContext& Context,
                                        const FStateTreeTransitionResult& Transition) const
{
	if (AOnsetBaseCharacter* Self = GetSelfBaseCharacter(Context))
	{
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
		Self->GetCharacterMovement()->MaxWalkSpeed = InstanceData.CachedOriginalWalkSpeed;
	}

	AOnsetAIController* AIController = GetController(Context);
	if (AIController) AIController->StopMovement();
}
