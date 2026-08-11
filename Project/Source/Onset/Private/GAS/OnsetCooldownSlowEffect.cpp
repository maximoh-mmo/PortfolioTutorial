// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/OnsetCooldownSlowEffect.h"

#include "GAS/OnsetCombatAttributeSet.h"

UOnsetCooldownSlowEffect::UOnsetCooldownSlowEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	// Re-casting refreshes the debuff instead of stacking multiplicatively
	// (1.0 * 2.0 * 2.0 = 4.0 would let spamming trivialize cooldowns).
	StackingType = EGameplayEffectStackingType::AggregateBySource;
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
	StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::ResetOnSuccessfulApplication;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UOnsetCombatAttributeSet::GetCooldownMultiplierAttribute();
	Modifier.ModifierOp = EGameplayModOp::MultiplyCompound;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataName = FName("CooldownRateMod");
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	Modifiers.Add(Modifier);

	FSetByCallerFloat DurationByCaller;
	DurationByCaller.DataName = FName("Duration");
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationByCaller);
}
