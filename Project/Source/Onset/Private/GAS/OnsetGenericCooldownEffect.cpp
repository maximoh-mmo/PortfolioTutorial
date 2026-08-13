// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/OnsetGenericCooldownEffect.h"

UOnsetGenericCooldownEffect::UOnsetGenericCooldownEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat DurationByCaller;
	DurationByCaller.DataName = FName("Duration");
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationByCaller);
}
