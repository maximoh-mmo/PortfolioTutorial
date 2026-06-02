#include "AI/Conditions/OnsetStateTreeDistanceCondition.h"

#include "StateTreeExecutionContext.h"
#include "AI/OnsetAIController.h"
#include "AI/Tasks/OnsetStateTreeTaskBase.h"

bool FOnsetStateTreeDistanceCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	FOnsetStateTreeDistanceConditionInstance& InstanceData = Context.GetInstanceData<FOnsetStateTreeDistanceConditionInstance>(*this);
	AOnsetAIController* AIController = Cast<AOnsetAIController>(Context.GetOwner());
	if (!AIController || AIController->GetPawn()) return false;
	
	FVector SourceLocation = AIController->GetPawn()->GetActorLocation();
	FVector TargetLocation;
	
	if (InstanceData.DistanceSource == EOnsetStateTreeDistanceSource::CurrentTarget)
	{
		AActor* Target = FOnsetStateTreeTaskBase::GetTarget(Context);
		if (!Target)
		{
			return InstanceData.bAllowNoTarget; // If no target is allowed, consider the condition true when there's no target.
		}
		TargetLocation = Target->GetActorLocation();
	}
	else
	{
		AOnsetBaseCharacter* Self = FOnsetStateTreeTaskBase::GetSelfBaseCharacter(Context);
		if (!Self) return false;
		TargetLocation = Self->HomeLocation;
	}
	
	float Distance = FVector::DistSquared(SourceLocation, TargetLocation);
	float DistanceThresholdSquared = InstanceData.DistanceThreshold * InstanceData.DistanceThreshold;
	
	switch (InstanceData.Comparison)
	{
	case UE::StateTree::EComparisonOperator::Less:
		return Distance < DistanceThresholdSquared;
	case UE::StateTree::EComparisonOperator::LessOrEqual:
		return Distance <= DistanceThresholdSquared;
	case UE::StateTree::EComparisonOperator::Greater:
		return Distance > DistanceThresholdSquared;
	case UE::StateTree::EComparisonOperator::GreaterOrEqual:
		return Distance >= DistanceThresholdSquared;
	default:
		return false;
	}
}
