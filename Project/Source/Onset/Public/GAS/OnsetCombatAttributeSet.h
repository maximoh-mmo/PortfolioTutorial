// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "OnsetCombatAttributeSet.generated.h"

/**
 * Combat attributes shared by player and NPC characters.
 *
 * CooldownMultiplier scales cooldown durations (base x CooldownMultiplier).
 * Default 1.0; the Slow debuff raises it above 1 via GE_GenericSlow
 * (UOnsetCooldownSlowEffect), so a slowed target's abilities recover more
 * slowly and it attacks less often.
 */
UCLASS()
class ONSET_API UOnsetCombatAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UOnsetCombatAttributeSet();

	UPROPERTY(ReplicatedUsing=OnRep_CooldownMultiplier, EditAnywhere, BlueprintReadWrite, Category="Attributes")
	FGameplayAttributeData CooldownMultiplier;

	ATTRIBUTE_ACCESSORS_BASIC(UOnsetCombatAttributeSet, CooldownMultiplier)

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	virtual void OnRep_CooldownMultiplier(const FGameplayAttributeData& OldCooldownMultiplier);
};
