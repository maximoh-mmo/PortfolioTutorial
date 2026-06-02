// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "OnsetBaseCharacter.generated.h"

class UOnsetAttributeSet;
class UAbilitySystemComponent;
/** Shared base for player and NPC characters. Used as a common type for targeting and ability systems. */
UCLASS(Blueprintable)
class ONSET_API AOnsetBaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AOnsetBaseCharacter();
	void InitAbilityActorInfo();
	virtual void PossessedBy(AController* NewController) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FVector HomeLocation;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	                                                                           
	UPROPERTY()                                                                                                
	TObjectPtr<UOnsetAttributeSet> AttributeSet;    
};
