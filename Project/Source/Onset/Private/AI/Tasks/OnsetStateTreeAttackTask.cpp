#include "AI/Tasks/OnsetStateTreeAttackTask.h"

#include "AbilitySystemComponent.h"
#include "StateTreeExecutionContext.h"
#include "AI/OnsetAIController.h"
#include "Combat/GameplayAbilities/OnsetGA_BasicAttack.h"
#include "GAS/OnsetGameplayTags.h"
#include "Player/OnsetBaseCharacter.h"
#include "Player/TargetingComponent.h"

EStateTreeRunStatus FOnsetStateTreeAttackTask::EnterState(FStateTreeExecutionContext& Context,
                                                          const FStateTreeTransitionResult& Transition) const
{
	AOnsetAIController* AIController = GetController(Context);
	if (!AIController) return EStateTreeRunStatus::Failed;
	AActor* Target = GetTarget(Context);
	if (!Target) return EStateTreeRunStatus::Failed;
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AIController->StopMovement();
	AOnsetBaseCharacter* Self = Cast<AOnsetBaseCharacter>(AIController->GetPawn());
	if (!Self || !Self->AbilitySystemComponent || !Self->TargetingComponent || !Self->TargetingComponent->GetTarget())
	{
		Self->AbilitySystemComponent->TryActivateAbility(
			Self->AbilitySystemComponent->FindAbilitySpecFromClass(UOnsetGA_BasicAttack::StaticClass())->Handle);
	}
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FOnsetStateTreeAttackTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.AbilityClass) return EStateTreeRunStatus::Failed;
	
	AOnsetBaseCharacter* Self = GetSelfBaseCharacter(Context);
	if (!Self || !Self->AbilitySystemComponent)	return EStateTreeRunStatus::Failed;
	
	if (Self->AbilitySystemComponent->HasMatchingGameplayTag(TAG_Cooldown_BasicAttack))
		return EStateTreeRunStatus::Running;
	
	Self->AbilitySystemComponent->TryActivateAbilityByClass(InstanceData.AbilityClass);
	
	if (!GetTarget(Context)) return EStateTreeRunStatus::Succeeded;
	
	return EStateTreeRunStatus::Running;
}
