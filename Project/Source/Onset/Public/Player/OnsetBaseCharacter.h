// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "OnsetBaseCharacter.generated.h"

UCLASS(Blueprintable)
class ONSET_API AOnsetBaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AOnsetBaseCharacter();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;
};
