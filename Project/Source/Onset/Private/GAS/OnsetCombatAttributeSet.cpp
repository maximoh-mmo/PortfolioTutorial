// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/OnsetCombatAttributeSet.h"

#include "GAS/OnsetGameplayTags.h"
#include "Net/UnrealNetwork.h"

UOnsetCombatAttributeSet::UOnsetCombatAttributeSet()
{
	// Shared baseline stats. Phase 2 (stat aggregation) overrides these per
	// character from class base + allocated points + gear.
	InitStrength(10.0f);
	InitIntellect(10.0f);
	InitVitality(10.0f);
	InitDefense(10.0f);
	InitResistanceFire(0.0f);
	InitResistanceIce(0.0f);
	InitResistanceLightning(0.0f);
	InitResistancePoison(0.0f);
	InitAgility(10.0f);
	InitLuck(10.0f);
	InitBlockChance(0.0f);
	InitCooldownMultiplier(1.0f);
	InitOutgoingDamageMod(0.0f);
	InitIncomingDamageMod(0.0f);
	InitPrestigeMultiplier(1.0f);
}

FGameplayAttribute UOnsetCombatAttributeSet::GetMitigationAttributeForDamageTag(const FGameplayTag& DamageType) const
{
	if (DamageType == TAG_Damage_Physical)
	{
		return GetDefenseAttribute();
	}
	if (DamageType == TAG_Damage_Fire)
	{
		return GetResistanceFireAttribute();
	}
	if (DamageType == TAG_Damage_Ice)
	{
		return GetResistanceIceAttribute();
	}
	if (DamageType == TAG_Damage_Lightning)
	{
		return GetResistanceLightningAttribute();
	}
	if (DamageType == TAG_Damage_Poison)
	{
		return GetResistancePoisonAttribute();
	}
	return FGameplayAttribute();
}

void UOnsetCombatAttributeSet::ResetToDefaults()
{
	InitStrength(10.0f);
	InitIntellect(10.0f);
	InitVitality(10.0f);
	InitDefense(10.0f);
	InitResistanceFire(0.0f);
	InitResistanceIce(0.0f);
	InitResistanceLightning(0.0f);
	InitResistancePoison(0.0f);
	InitAgility(10.0f);
	InitLuck(10.0f);
	InitBlockChance(0.0f);
	InitCooldownMultiplier(1.0f);
	InitOutgoingDamageMod(0.0f);
	InitIncomingDamageMod(0.0f);
	InitPrestigeMultiplier(1.0f);
}

void UOnsetCombatAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UOnsetCombatAttributeSet, Strength, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOnsetCombatAttributeSet, Intellect, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOnsetCombatAttributeSet, Vitality, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOnsetCombatAttributeSet, Defense, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOnsetCombatAttributeSet, ResistanceFire, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOnsetCombatAttributeSet, ResistanceIce, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOnsetCombatAttributeSet, ResistanceLightning, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOnsetCombatAttributeSet, ResistancePoison, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOnsetCombatAttributeSet, Agility, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOnsetCombatAttributeSet, Luck, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOnsetCombatAttributeSet, BlockChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOnsetCombatAttributeSet, CooldownMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOnsetCombatAttributeSet, OutgoingDamageMod, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOnsetCombatAttributeSet, IncomingDamageMod, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOnsetCombatAttributeSet, PrestigeMultiplier, COND_None, REPNOTIFY_Always);
}

void UOnsetCombatAttributeSet::OnRep_Strength(const FGameplayAttributeData& OldStrength)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOnsetCombatAttributeSet, Strength, OldStrength);
}

void UOnsetCombatAttributeSet::OnRep_Intellect(const FGameplayAttributeData& OldIntellect)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOnsetCombatAttributeSet, Intellect, OldIntellect);
}

void UOnsetCombatAttributeSet::OnRep_Vitality(const FGameplayAttributeData& OldVitality)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOnsetCombatAttributeSet, Vitality, OldVitality);
}

void UOnsetCombatAttributeSet::OnRep_Defense(const FGameplayAttributeData& OldDefense)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOnsetCombatAttributeSet, Defense, OldDefense);
}

void UOnsetCombatAttributeSet::OnRep_ResistanceFire(const FGameplayAttributeData& OldResistanceFire)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOnsetCombatAttributeSet, ResistanceFire, OldResistanceFire);
}

void UOnsetCombatAttributeSet::OnRep_ResistanceIce(const FGameplayAttributeData& OldResistanceIce)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOnsetCombatAttributeSet, ResistanceIce, OldResistanceIce);
}

void UOnsetCombatAttributeSet::OnRep_ResistanceLightning(const FGameplayAttributeData& OldResistanceLightning)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOnsetCombatAttributeSet, ResistanceLightning, OldResistanceLightning);
}

void UOnsetCombatAttributeSet::OnRep_ResistancePoison(const FGameplayAttributeData& OldResistancePoison)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOnsetCombatAttributeSet, ResistancePoison, OldResistancePoison);
}

void UOnsetCombatAttributeSet::OnRep_Agility(const FGameplayAttributeData& OldAgility)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOnsetCombatAttributeSet, Agility, OldAgility);
}

void UOnsetCombatAttributeSet::OnRep_Luck(const FGameplayAttributeData& OldLuck)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOnsetCombatAttributeSet, Luck, OldLuck);
}

void UOnsetCombatAttributeSet::OnRep_BlockChance(const FGameplayAttributeData& OldBlockChance)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOnsetCombatAttributeSet, BlockChance, OldBlockChance);
}

void UOnsetCombatAttributeSet::OnRep_CooldownMultiplier(const FGameplayAttributeData& OldCooldownMultiplier)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOnsetCombatAttributeSet, CooldownMultiplier, OldCooldownMultiplier);
}

void UOnsetCombatAttributeSet::OnRep_OutgoingDamageMod(const FGameplayAttributeData& OldOutgoingDamageMod)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOnsetCombatAttributeSet, OutgoingDamageMod, OldOutgoingDamageMod);
}

void UOnsetCombatAttributeSet::OnRep_IncomingDamageMod(const FGameplayAttributeData& OldIncomingDamageMod)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOnsetCombatAttributeSet, IncomingDamageMod, OldIncomingDamageMod);
}

void UOnsetCombatAttributeSet::OnRep_PrestigeMultiplier(const FGameplayAttributeData& OldPrestigeMultiplier)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOnsetCombatAttributeSet, PrestigeMultiplier, OldPrestigeMultiplier);
}