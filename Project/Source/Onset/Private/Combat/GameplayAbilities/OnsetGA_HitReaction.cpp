// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/GameplayAbilities/OnsetGA_HitReaction.h"

#include "Combat/OnsetGameplayTags.h"

UOnsetGA_HitReaction::UOnsetGA_HitReaction()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = TAG_Event_HitReaction;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UOnsetGA_HitReaction::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}
	
	// Apply cooldown GE (blocks re-triggering)
	if (CooldownGameplayEffectClass)
	{
		// void to avoid IDE warnings for unused statement result
		(void)ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, CooldownGameplayEffectClass, GetAbilityLevel());
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
