// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "OnsetMovementAttributeSet.generated.h"

// Macro for boilerplate OnRep replication
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)
/**
 * 
 */
UCLASS()
class ONSET_API UOnsetMovementAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	UOnsetMovementAttributeSet();
	
	UPROPERTY(ReplicatedUsing=OnRep_MovementSpeed, EditAnywhere, BlueprintReadWrite, Category="Attributes")
	FGameplayAttributeData MovementSpeed;
	
	ATTRIBUTE_ACCESSORS(UOnsetMovementAttributeSet, MovementSpeed)
	
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION()
	virtual void OnRep_MovementSpeed(const FGameplayAttributeData& OldMovementSpeed);
	
};
