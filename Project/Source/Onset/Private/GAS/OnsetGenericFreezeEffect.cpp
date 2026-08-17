// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/OnsetGenericFreezeEffect.h"

#include "GAS/OnsetGameplayTags.h"

UOnsetGenericFreezeEffect::UOnsetGenericFreezeEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	// Re-casting refreshes the freeze instead of stacking.
	StackingType = EGameplayEffectStackingType::AggregateBySource;
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
	StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::ResetOnSuccessfulApplication;

	// Grants State.Frozen for the duration window. The engine migrates this to a
	// UTargetTagsGameplayEffectComponent at PostCDOCompiled (the component API can't
	// be used in the constructor: NewObject with an empty name is illegal there).
	InheritableOwnedTagsContainer.AddTag(TAG_State_Frozen);

	FSetByCallerFloat DurationByCaller;
	DurationByCaller.DataName = FName("Duration");
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationByCaller);
}