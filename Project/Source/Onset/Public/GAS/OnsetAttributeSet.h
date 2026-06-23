// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "OnsetAttributeSet.generated.h"

// Macro for boilerplate OnRep replication
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

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
	
	ATTRIBUTE_ACCESSORS(UOnsetAttributeSet, Health)
	ATTRIBUTE_ACCESSORS(UOnsetAttributeSet, MaxHealth)
	
	// Clamps Health to [0, MaxHealth] after any GameplayEffect
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);
	
	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);
	
	
};
