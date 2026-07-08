// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "UI/OnsetRootLayout.h"
#include "OnsetGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class ONSET_API UOnsetGameInstance : public UGameInstance
{
	GENERATED_BODY()  
public:
	virtual void Init() override;                                                                               
	UPROPERTY(EditDefaultsOnly)                                                                                 
	TSubclassOf<UOnsetRootLayout> RootLayoutClass;  
};
