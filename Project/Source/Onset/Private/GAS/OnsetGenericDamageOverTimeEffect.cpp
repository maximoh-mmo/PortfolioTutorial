// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/OnsetGenericDamageOverTimeEffect.h"

#include "Combat/UOnsetDamageExecution.h"

UOnsetGenericDamageOverTimeEffect::UOnsetGenericDamageOverTimeEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat DurationByCaller;
	DurationByCaller.DataName = FName("Duration");
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationByCaller);

	// Authoring fallback; the ability overrides spec.Period from the row's Period.
	Period.SetValue(1.0f);

	// The execution outputs the per-tick Health modifier; no static modifiers here.
	FGameplayEffectExecutionDefinition ExecutionDefinition;
	ExecutionDefinition.CalculationClass = UOnsetDamageExecution::StaticClass();
	Executions.Add(ExecutionDefinition);
}