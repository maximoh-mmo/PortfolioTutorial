#include "StateTree/Conditions/OnsetStateTreeDistanceCondition.h"

#include "StateTreeExecutionContext.h"
#include "Enemy/Profile/AIProfile.h"
#include "AI/OnsetAIController.h"
#include "StateTree/Tasks/OnsetStateTreeTask.h"
#include "Core/OnsetBaseCharacter.h"

bool FOnsetStateTreeDistanceCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	FOnsetStateTreeDistanceConditionInstance& InstanceData = Context.GetInstanceData<FOnsetStateTreeDistanceConditionInstance>(*this);
	AOnsetAIController* AIController = Cast<AOnsetAIController>(Context.GetOwner());
	if (!AIController || !AIController->GetPawn()) return false;
	
	FVector SourceLocation = AIController->GetPawn()->GetActorLocation();
	FVector TargetLocation;
	
	// Resolve target location
	if (InstanceData.DistanceSource == EOnsetStateTreeDistanceSource::CurrentTarget ||
		InstanceData.DistanceSource == EOnsetStateTreeDistanceSource::AttackRange ||
		InstanceData.DistanceSource == EOnsetStateTreeDistanceSource::ChaseRange)
	{
		AActor* Target = FOnsetStateTreeTask::GetTarget(Context);
		// If no target is allowed, consider the condition true when there's no target.
		if (!Target) return InstanceData.bAllowNoTarget;
		TargetLocation = Target->GetActorLocation();
	}
	else // Home location
	{
		AOnsetBaseCharacter* Self = FOnsetStateTreeTask::GetSelfPawn<AOnsetBaseCharacter>(Context);
		if (!Self) return false;
		TargetLocation = Self->HomeTransform.GetLocation();
	}
	
	// Resolve threshold
	float DistanceThresholdSquared;
	if (InstanceData.DistanceSource == EOnsetStateTreeDistanceSource::AttackRange ||
		InstanceData.DistanceSource == EOnsetStateTreeDistanceSource::ChaseRange)
	{
		if (!AIController->GetAIProfile()) return false;
		float Threshold = InstanceData.DistanceSource == EOnsetStateTreeDistanceSource::AttackRange 
			                  ? AIController->GetAIProfile()->AttackRange 
			                  : AIController->GetAIProfile()->ChaseRange;
		DistanceThresholdSquared = Threshold * Threshold;
	}
	else if (InstanceData.DistanceSource == EOnsetStateTreeDistanceSource::HomeLocation &&
	         InstanceData.DistanceThreshold <= 0.0f &&
	         AIController->GetAIProfile())
	{
		// When measuring from HomeLocation without an explicit override, default to the profile's ChaseRange (leash distance)
		const float Threshold = AIController->GetAIProfile()->ChaseRange;
		DistanceThresholdSquared = Threshold * Threshold;
	}
	else
	{
		DistanceThresholdSquared = InstanceData.DistanceThreshold * InstanceData.DistanceThreshold;
	}
	const float DistanceSquared = FVector::DistSquared(SourceLocation, TargetLocation);
	switch (InstanceData.Comparison)
	{
	case UE::StateTree::EComparisonOperator::Less:
		return DistanceSquared < DistanceThresholdSquared;
	case UE::StateTree::EComparisonOperator::LessOrEqual:
		return DistanceSquared <= DistanceThresholdSquared;
	case UE::StateTree::EComparisonOperator::Greater:
		return DistanceSquared > DistanceThresholdSquared;
	case UE::StateTree::EComparisonOperator::GreaterOrEqual:
		return DistanceSquared >= DistanceThresholdSquared;
	default:
		return false;
	}
}
