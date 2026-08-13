// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "OnsetAbilityRowButton.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAbilityRowSelected, FName);

/**
 * Row button used in the ability list. Carries its RowName and broadcasts selection
 * (a plain multicast so the editor widget can bind to a per-row handler).
 */
UCLASS()
class UOnsetAbilityRowButton : public UButton
{
	GENERATED_BODY()

public:
	FName RowName;
	FOnAbilityRowSelected OnRowSelected;

	UFUNCTION()
	void HandleClicked() { OnRowSelected.Broadcast(RowName); }
};