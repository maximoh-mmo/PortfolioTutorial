// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/OnsetGenericHealOverTimeEffect.h"

#include "GAS/OnsetAttributeSet.h"

UOnsetGenericHealOverTimeEffect::UOnsetGenericHealOverTimeEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat DurationByCaller;
	DurationByCaller.DataName = FName("Duration");
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationByCaller);

	// Authoring fallback; the ability overrides spec.Period from the row's Period.
	Period.SetValue(1.0f);

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UOnsetAttributeSet::GetHealthAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat HealByCaller;
	HealByCaller.DataName = FName("HealAmount");
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(HealByCaller);

	Modifiers.Add(Modifier);
}