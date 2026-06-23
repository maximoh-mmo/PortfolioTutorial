// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "OnsetMovementAttributeSet.generated.h"

/** Movement speed attribute set with OnRep replication. */
UCLASS()
class ONSET_API UOnsetMovementAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	UOnsetMovementAttributeSet();
	
	UPROPERTY(ReplicatedUsing=OnRep_MovementSpeed, EditAnywhere, BlueprintReadWrite, Category="Attributes")
	FGameplayAttributeData MovementSpeed;
	
	ATTRIBUTE_ACCESSORS_BASIC(UOnsetMovementAttributeSet, MovementSpeed)
	
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION()
	virtual void OnRep_MovementSpeed(const FGameplayAttributeData& OldMovementSpeed);
	
};
