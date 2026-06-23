// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/OnsetGA_HitReaction.h"

#include "GAS/OnsetGameplayTags.h"
#include "UObject/ConstructorHelpers.h"

UOnsetGA_HitReaction::UOnsetGA_HitReaction()
{
	static ConstructorHelpers::FObjectFinder<UGameplayEffect> StaggerFinder(
		TEXT("Game/Game/Combat/GE_Stagger.GE_Stagger_C"));
	if (StaggerFinder.Succeeded())
	{
		StaggerEffect = StaggerFinder.Object;
	}
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
		if (StaggerEffect)
		{
			ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, StaggerEffect, GetAbilityLevel());
		}
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
