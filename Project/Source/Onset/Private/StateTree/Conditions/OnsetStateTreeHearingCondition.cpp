#include "StateTree/Conditions/OnsetStateTreeHearingCondition.h"
#include "StateTreeExecutionContext.h"
#include "AI/OnsetAIController.h"
#include "StateTree/Tasks/OnsetStateTreeTask.h"
#include "Engine/World.h"

bool FOnsetStateTreeHearingCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FOnsetStateTreeHearingConditionInstanceData& InstanceData = Context.GetInstanceData(*this);
	
	AOnsetAIController *Controller = FOnsetStateTreeTask::GetController(Context);
	if (!Controller || !Controller->bHasPendingNoise) return false;
	
	// Noise expired? (No new noise within MaxTimeSinceLastNoise seconds as set in the InstanceData)
	float TimeSinceLastHeardNoise = Controller->GetWorld()->GetTimeSeconds() - Controller->LastNoiseHeardTime;
	if (TimeSinceLastHeardNoise > InstanceData.MaxTimeSinceLastNoise) return false;
	return true;	
}
