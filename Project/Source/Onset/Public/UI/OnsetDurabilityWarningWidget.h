// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/OnsetEquipmentTypes.h"
#include "OnsetDurabilityWarningWidget.generated.h"

class UOnsetInventoryComponent;
class UImage;
class UCommonTextBlock;

/**
 * Tactical HUD warning indicator that alerts the player when equipped weapons
 * or armor pieces fall below 20% durability or enter the Malfunctioning state.
 */
UCLASS(Abstract, Blueprintable)
class ONSET_API UOnsetDurabilityWarningWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Binds this warning widget to the inventory component. */
	UFUNCTION(BlueprintCallable, Category = "Onset|Durability")
	void BindToInventory(UOnsetInventoryComponent* InInventory);

protected:
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> WarningIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> StatusText;

	UFUNCTION(BlueprintImplementableEvent, Category = "Onset|Durability")
	void BP_OnDurabilityStatusChanged(bool bHasBrokenItems, bool bHasLowItems);

private:
	void HandleInventoryChanged();

	TWeakObjectPtr<UOnsetInventoryComponent> BoundInventory;
};
