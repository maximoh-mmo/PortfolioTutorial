// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/OnsetGenericInvulnerableEffect.h"

#include "GAS/OnsetGameplayTags.h"

UOnsetGenericInvulnerableEffect::UOnsetGenericInvulnerableEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	// Re-casting refreshes the window instead of stacking.
	StackingType = EGameplayEffectStackingType::AggregateBySource;
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
	StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::ResetOnSuccessfulApplication;

	// Grants State.Invulnerable for the duration window. The engine migrates this to a
	// UTargetTagsGameplayEffectComponent at PostCDOCompiled (the component API can't
	// be used in the constructor: NewObject with an empty name is illegal there).
	InheritableOwnedTagsContainer.AddTag(TAG_State_Invulnerable);

	FSetByCallerFloat DurationByCaller;
	DurationByCaller.DataName = FName("Duration");
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationByCaller);
}