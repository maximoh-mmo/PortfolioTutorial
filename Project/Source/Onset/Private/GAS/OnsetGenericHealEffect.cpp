// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/OnsetGenericHealEffect.h"

#include "GAS/OnsetAttributeSet.h"

UOnsetGenericHealEffect::UOnsetGenericHealEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UOnsetAttributeSet::GetHealthAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataName = FName("HealAmount");
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	Modifiers.Add(Modifier);
}