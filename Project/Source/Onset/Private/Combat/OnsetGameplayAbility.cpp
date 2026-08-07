// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/OnsetGameplayAbility.h"

#include "GameplayTagContainer.h"

FGameplayTag UOnsetGameplayAbility::GetPrimaryCooldownTag() const
{
	const FGameplayTagContainer* CooldownTags = GetCooldownTags();
	if (CooldownTags && CooldownTags->Num() > 0)
	{
		return CooldownTags->First();
	}
	return FGameplayTag();
}
