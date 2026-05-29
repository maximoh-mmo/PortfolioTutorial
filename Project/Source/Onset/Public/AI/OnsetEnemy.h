// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/OnsetBaseCharacter.h"
#include "OnsetEnemy.generated.h"

class UGroupComponent;
/**
 * 
 */
UCLASS()
class ONSET_API AOnsetEnemy : public AOnsetBaseCharacter
{
	GENERATED_BODY()
	
public:
	/**
	 * 
	 */
	AOnsetEnemy();
	
	UPROPERTY()                                                                                                     
	UGroupComponent* GroupComp;     
};
