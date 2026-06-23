// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "OnsetGA_HitReaction.generated.h"

UCLASS()
class ONSET_API UOnsetGA_HitReaction : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UOnsetGA_HitReaction();
	
	UPROPERTY()
	TObjectPtr<UGameplayEffect> StaggerEffect;
protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo, 
		const FGameplayEventData* TriggerEventData) override;	
};
