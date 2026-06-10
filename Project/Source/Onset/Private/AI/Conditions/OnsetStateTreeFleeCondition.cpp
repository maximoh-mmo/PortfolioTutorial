#include "AI/Conditions/OnsetStateTreeFleeCondition.h"                                                                  
#include "StateTreeExecutionContext.h"
#include "AI/OnsetAIController.h"
#include "Enemy/Profile/AIProfile.h"
#include "AI/Tasks/OnsetStateTreeTaskBase.h"
#include "GAS/OnsetAttributeSet.h"
#include "Enemy/GroupComponent.h"
#include "Enemy/OnsetEnemy.h"

bool FOnsetStateTreeFleeCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	FOnsetFleeConditionInstanceData& InstanceData = Context.GetInstanceData<FOnsetFleeConditionInstanceData>(*this);
	AOnsetAIController* AIController = FOnsetStateTreeTaskBase::GetController(Context); 
	if (!AIController->GetAIProfile()) return false;
	AOnsetEnemy* Self = FOnsetStateTreeTaskBase::GetSelfEnemyCharacter(Context);
	if (!Self || !Self->AbilitySystemComponent) return false;
	
	float MaxHealth = Self->AttributeSet->GetMaxHealth();
	if (MaxHealth <= 0.0f) return false;
	
	float HealthRatio = Self->AttributeSet->GetHealth() / MaxHealth;
	float Threshold = AIController->GetAIProfile()->FleeThreshold;
	if (Threshold <= 0.0f) return false;
	
	if (Self->GroupComp && Self->GroupComp->IsInGroup())
	{
		FGroupData GroupData = Self->GroupComp->GetGroupData();
		float DistToCenterSq = FVector::DistSquared(Self->GetActorLocation(), GroupData.Center);
		if (DistToCenterSq < InstanceData.GroupSupportRadius*InstanceData.GroupSupportRadius)
		{
			int32 AllyCount = FMath::Max(0, GroupData.AliveCount - 1);
			Threshold -= AllyCount * InstanceData.GroupCouragePerAlly;
		}
	}
	
	if (HealthRatio > Threshold) return false;
	
	return FMath::FRand() <= InstanceData.FleeProbability;
}
