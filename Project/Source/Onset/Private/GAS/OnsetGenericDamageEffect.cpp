// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/OnsetGenericDamageEffect.h"

#include "Combat/UOnsetDamageExecution.h"

UOnsetGenericDamageEffect::UOnsetGenericDamageEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// The execution outputs the Health modifier; no static modifiers are declared here.
	FGameplayEffectExecutionDefinition ExecutionDefinition;
	ExecutionDefinition.CalculationClass = UOnsetDamageExecution::StaticClass();
	Executions.Add(ExecutionDefinition);
}
