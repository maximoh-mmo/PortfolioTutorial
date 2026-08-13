// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/OnsetGenericSnareEffect.h"

#include "GAS/OnsetMovementAttributeSet.h"

UOnsetGenericSnareEffect::UOnsetGenericSnareEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	// Re-casting refreshes the snare instead of stacking multiplicatively.
	StackingType = EGameplayEffectStackingType::AggregateBySource;
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
	StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::ResetOnSuccessfulApplication;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UOnsetMovementAttributeSet::GetMovementSpeedAttribute();
	Modifier.ModifierOp = EGameplayModOp::MultiplyCompound;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataName = FName("MoveSpeedMod");
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	Modifiers.Add(Modifier);

	FSetByCallerFloat DurationByCaller;
	DurationByCaller.DataName = FName("Duration");
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationByCaller);
}
