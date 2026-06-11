#include "StateTree/Tasks/Enemy/FleeTask.h"
#include "NavigationSystem.h"
#include "StateTreeExecutionContext.h"
#include "Enemy/OnsetAIController.h"
#include "GAS/OnsetAttributeSet.h"
#include "Navigation/PathFollowingComponent.h"
#include "Player/OnsetBaseCharacter.h"                                                            
                                                         
EStateTreeRunStatus FFleeTask::EnterState(FStateTreeExecutionContext& Context,
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
		float InitialHealthRatio = Self->AttributeSet->GetHealth() / Self->AttributeSet->GetMaxHealth();                
		float InitialSpeedMod = FMath::Lerp(InstanceData.MinSpeedMultiplier, 1.0f, InitialHealthRatio);                 
		InstanceData.SpeedEffectHandle = ApplyMovementSpeedModifier(                           
			Self, InitialSpeedMod);
		AIController->MoveToLocation(InstanceData.FleeDestination, InstanceData.AcceptanceRadius);
		return EStateTreeRunStatus::Running;
	}

	return EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FFleeTask::Tick(FStateTreeExecutionContext& Context,
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
		if (InstanceData.SpeedEffectHandle.IsValid())                                                                   
		{                                                                                                               
			Self->AbilitySystemComponent->RemoveActiveGameplayEffect(InstanceData.SpeedEffectHandle);                   
		}  
		
		InstanceData.SpeedEffectHandle = ApplyMovementSpeedModifier(                           
			Self, SpeedMod);
	}

	return HasMoveCompleted(Context) ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

void FFleeTask::ExitState(FStateTreeExecutionContext& Context,
                                        const FStateTreeTransitionResult& Transition) const
{
	if (AOnsetBaseCharacter* Self = GetSelfBaseCharacter(Context))
	{
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
		if (InstanceData.SpeedEffectHandle.IsValid())                                                                   
		{                                                                                                               
			Self->AbilitySystemComponent->RemoveActiveGameplayEffect(InstanceData.SpeedEffectHandle);                   
			InstanceData.SpeedEffectHandle.Invalidate();                                                                
		}        
	}

	AOnsetAIController* AIController = GetController(Context);
	if (AIController) AIController->StopMovement();
}
