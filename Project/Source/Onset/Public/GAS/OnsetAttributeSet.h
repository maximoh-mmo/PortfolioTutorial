// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "OnsetAttributeSet.generated.h"

/** Base attribute set for health, damage, and death events. */
UCLASS()
class ONSET_API UOnsetAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UOnsetAttributeSet();
	
	UPROPERTY(ReplicatedUsing=OnRep_Health, EditAnywhere, BlueprintReadWrite, Category="Attributes")
	FGameplayAttributeData Health;
	
	UPROPERTY(ReplicatedUsing=OnRep_MaxHealth, EditAnywhere, BlueprintReadWrite, Category="Attributes")
	FGameplayAttributeData MaxHealth;
	
	ATTRIBUTE_ACCESSORS_BASIC(UOnsetAttributeSet, Health)
	ATTRIBUTE_ACCESSORS_BASIC(UOnsetAttributeSet, MaxHealth)
	
	// Clamps Health to [0, MaxHealth] after any GameplayEffect
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);
	
	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);	
};
