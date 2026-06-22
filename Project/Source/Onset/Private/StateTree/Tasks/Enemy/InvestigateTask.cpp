// Fill out your copyright notice in the Description page of Project Settings.


#include "StateTree/Tasks/Enemy/InvestigateTask.h"

#include "AbilitySystemComponent.h"
#include "StateTreeExecutionContext.h"
#include "AI/OnsetAIController.h"
#include "Enemy/GroupComponent.h"
#include "Core/OnsetBaseCharacter.h"

EStateTreeRunStatus FInvestigateTask::EnterState(FStateTreeExecutionContext& Context,
                                                               const FStateTreeTransitionResult& Transition) const
{
	AOnsetAIController* Controller = GetController(Context);
	if (!Controller) return	EStateTreeRunStatus::Failed;
	
	AOnsetBaseCharacter* Self = GetSelfBaseCharacter(Context);
	if (!Self) return	EStateTreeRunStatus::Failed;
	
	if (!Controller->bHasPendingNoise) return EStateTreeRunStatus::Succeeded;

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.InvestigationDestination = Controller->HeardNoiseLocation;
	
	UGroupComponent* GroupComponent = Self->FindComponentByClass<UGroupComponent>();
	bool bIsGroupMember = false;
	if (GroupComponent && GroupComponent->IsInGroup())
	{
		if (AActor* InstigatorActor = Controller->HeardNoiseInstigator.Get())
		{
			UGroupComponent* TheirGroupComponent = InstigatorActor->FindComponentByClass<UGroupComponent>();
			bIsGroupMember = TheirGroupComponent && TheirGroupComponent->GetGroupManager() == GroupComponent->GetGroupManager();
		}
	}
	float MovementMultiplier = bIsGroupMember ? InstanceData.GroupMemberSpeedMultiplier 
												: InstanceData.NonGroupSpeedMultiplier;
	
	InstanceData.SpeedEffectHandle = ApplyMovementSpeedModifier(                           
         Self, MovementMultiplier);  	
	Controller->MoveToLocation(InstanceData.InvestigationDestination, InstanceData.AcceptanceRadius);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FInvestigateTask::Tick(FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	AOnsetAIController* Controller = GetController(Context);
	if (!Controller) return	EStateTreeRunStatus::Failed;
	
	if (GetTarget(Context) || !Controller->bHasPendingNoise) return EStateTreeRunStatus::Succeeded;
	
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.InvestigationDestination.Equals(Controller->HeardNoiseLocation))
	{
		InstanceData.InvestigationDestination = Controller->HeardNoiseLocation;
		Controller->MoveToLocation(InstanceData.InvestigationDestination, InstanceData.AcceptanceRadius);
	}
	return HasMoveCompleted(Context) ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

void FInvestigateTask::ExitState(FStateTreeExecutionContext& Context,
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
	if (AOnsetAIController* Controller = GetController(Context))
	{
		Controller->StopMovement();
	}
}
