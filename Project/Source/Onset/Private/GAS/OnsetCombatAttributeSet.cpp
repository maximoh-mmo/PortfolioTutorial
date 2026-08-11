// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/OnsetCombatAttributeSet.h"

#include "Net/UnrealNetwork.h"

UOnsetCombatAttributeSet::UOnsetCombatAttributeSet()
{
	InitCooldownMultiplier(1.0f);
}

void UOnsetCombatAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UOnsetCombatAttributeSet, CooldownMultiplier, COND_None, REPNOTIFY_Always);
}

void UOnsetCombatAttributeSet::OnRep_CooldownMultiplier(const FGameplayAttributeData& OldCooldownMultiplier)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOnsetCombatAttributeSet, CooldownMultiplier, OldCooldownMultiplier);
}
