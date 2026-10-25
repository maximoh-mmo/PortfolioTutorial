// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/OnsetEquipmentTypes.h"
#include "Data/OnsetItemTypes.h"
#include "OnsetItemActionMenuWidget.generated.h"

class UOnsetButtonBase;
class UCommonTextBlock;
class UOnsetInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnsetItemActionExecuted);

/**
 * Context action menu popup providing Equip, Unequip, Salvage, Infusion, and Repair
 * buttons when an inventory or equipment cell is clicked.
 */
UCLASS(Abstract, Blueprintable)
class ONSET_API UOnsetItemActionMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Configures the menu for an item in the bag at BagSlotIndex. */
	UFUNCTION(BlueprintCallable, Category = "Onset|Inventory")
	void SetupForBagItem(UOnsetInventoryComponent* InInventory, int32 InBagSlotIndex);

	/** Configures the menu for an equipped item in EquipSlot. */
	UFUNCTION(BlueprintCallable, Category = "Onset|Inventory")
	void SetupForEquippedItem(UOnsetInventoryComponent* InInventory, EOnsetEquipmentSlot InEquipSlot);

	/** Closes and hides the action menu. */
	UFUNCTION(BlueprintCallable, Category = "Onset|Inventory")
	void CloseActionMenu();

	UPROPERTY(BlueprintAssignable, Category = "Onset|Inventory")
	FOnsetItemActionExecuted OnActionExecuted;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOnsetButtonBase> EquipButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOnsetButtonBase> UnequipButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOnsetButtonBase> SalvageButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOnsetButtonBase> InfuseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOnsetButtonBase> RepairButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOnsetButtonBase> CloseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> ActionPromptText;

private:
	UFUNCTION()
	void HandleEquipClicked();

	UFUNCTION()
	void HandleUnequipClicked();

	UFUNCTION()
	void HandleSalvageClicked();

	UFUNCTION()
	void HandleInfuseClicked();

	UFUNCTION()
	void HandleRepairClicked();

	UFUNCTION()
	void HandleCloseClicked();

	TWeakObjectPtr<UOnsetInventoryComponent> Inventory;
	int32 BagSlotIndex = INDEX_NONE;
	bool bIsEquipped = false;
	EOnsetEquipmentSlot EquipSlot = EOnsetEquipmentSlot::Weapon;
};
