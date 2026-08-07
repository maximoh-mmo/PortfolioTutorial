// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Combat/OnsetGameplayAbility.h"
#include "OnsetGA_HitReaction.generated.h"

/** Hit reaction ability: plays a stagger effect in response to taking damage. */
UCLASS()
class ONSET_API UOnsetGA_HitReaction : public UOnsetGameplayAbility
{
	GENERATED_BODY()
public:
	UOnsetGA_HitReaction();
	
	/** Stagger gameplay effect applied on hit. */
	UPROPERTY()
	TObjectPtr<UGameplayEffect> StaggerEffect;
protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo, 
		const FGameplayEventData* TriggerEventData) override;	
};
