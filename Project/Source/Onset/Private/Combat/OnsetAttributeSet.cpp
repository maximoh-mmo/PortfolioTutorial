// Fill out your copyright notice in the Description page of Project Settings.
#include "Combat/OnsetAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UOnsetAttributeSet::UOnsetAttributeSet()
{
	InitHealth(100.0f);
	InitMaxHealth(100.0f);
}

void UOnsetAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		// Clamp Health to [0, MaxHealth]
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth())); 
	}
}

void UOnsetAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UOnsetAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOnsetAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
}

void UOnsetAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOnsetAttributeSet, MaxHealth, OldMaxHealth);
}

void UOnsetAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOnsetAttributeSet, Health, OldHealth);
}

