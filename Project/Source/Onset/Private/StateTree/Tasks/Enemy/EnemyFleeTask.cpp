#include "StateTree/Tasks/Enemy/EnemyFleeTask.h"
#include "NavigationSystem.h"
#include "StateTreeExecutionContext.h"
#include "AI/OnsetAIController.h"
#include "Enemy/OnsetEnemy.h"
#include "GAS/OnsetAttributeSet.h"
#include "Navigation/PathFollowingComponent.h"
#include "Core/OnsetBaseCharacter.h"                                                            
#include "Subsystem/OnsetThreatSubsystem.h"
                                                          
EStateTreeRunStatus FEnemyFleeTask::EnterState(FStateTreeExecutionContext& Context,
                                                         const FStateTreeTransitionResult& Transition) const
{
	AOnsetAIController* AIController = GetController(Context);
	if (!AIController) return EStateTreeRunStatus::Failed;
	if (!AIController->HasAuthority()) return EStateTreeRunStatus::Failed;
	AIController->ClearFocus(EAIFocusPriority::Gameplay);

	if (UOnsetThreatSubsystem* Subsystem = GetThreatSubsystem(Context))
	{
		if (AOnsetEnemy* SelfEnemy = GetSelfPawn<AOnsetEnemy>(Context))
			Subsystem->UnregisterEngaged(SelfEnemy);
	}

	AOnsetBaseCharacter* Self = GetSelfPawn<AOnsetBaseCharacter>(Context);
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
		InstanceData.LastSpeedModifier = InitialSpeedMod;                                           
		InstanceData.SpeedEffectHandle = ApplyMovementSpeedModifier(                           
			Self, InitialSpeedMod);
		AIController->MoveToLocation(InstanceData.FleeDestination, InstanceData.AcceptanceRadius);
		return EStateTreeRunStatus::Running;
	}

	return EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FEnemyFleeTask::Tick(FStateTreeExecutionContext& Context,
                                                  const float DeltaTime) const
{
	AOnsetAIController* AIController = GetController(Context);
	if (!AIController) return EStateTreeRunStatus::Failed;
	if (!AIController->HasAuthority()) return EStateTreeRunStatus::Failed;
	if (!GetPathFollowingComponent(Context)) return EStateTreeRunStatus::Failed;

	if (AOnsetBaseCharacter* Self = GetSelfPawn<AOnsetBaseCharacter>(Context))
	{
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
		float HealthRatio = Self->AttributeSet->GetHealth() / Self->AttributeSet->GetMaxHealth();
		float SpeedMod = FMath::Lerp(InstanceData.MinSpeedMultiplier, 1.0f, HealthRatio);

		// Only reapply when the modifier actually changes (health changed)
		if (!FMath::IsNearlyEqual(InstanceData.LastSpeedModifier, SpeedMod, 0.001f))
		{
			if (InstanceData.SpeedEffectHandle.IsValid())
			{
				Self->AbilitySystemComponent->RemoveActiveGameplayEffect(InstanceData.SpeedEffectHandle);
			}
			InstanceData.LastSpeedModifier = SpeedMod;
			InstanceData.SpeedEffectHandle = ApplyMovementSpeedModifier(
				Self, SpeedMod);
		}
	}

	return HasMoveCompleted(Context) ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

void FEnemyFleeTask::ExitState(FStateTreeExecutionContext& Context,
                                        const FStateTreeTransitionResult& Transition) const
{
	if (AOnsetBaseCharacter* Self = GetSelfPawn<AOnsetBaseCharacter>(Context))
	{
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
		if (InstanceData.SpeedEffectHandle.IsValid())                                                                   
		{                                                                                                               
			Self->AbilitySystemComponent->RemoveActiveGameplayEffect(InstanceData.SpeedEffectHandle);                   
			InstanceData.SpeedEffectHandle.Invalidate();                                                                
			InstanceData.LastSpeedModifier = -1.0f;                                                                     
		}        
	}

	AOnsetAIController* AIController = GetController(Context);
	if (AIController) AIController->StopMovement();
}
