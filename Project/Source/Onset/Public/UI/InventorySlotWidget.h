// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/OnsetEquipmentTypes.h"
#include "Data/OnsetItemTypes.h"
#include "InventorySlotWidget.generated.h"

class UCommonTextBlock;
class UImage;
class UOnsetButtonBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnsetInventorySlotClicked, int32, SlotIndex);

/**
 * A single inventory cell. One class serves both the bag grid and the equipped
 * loadout: SetItem()/SetEmpty() drive bag cells, SetEquipment()/SetEquipmentEmpty()
 * drive the loadout cells. Visually styled in a Widget Blueprint (WBP_InventorySlot);
 * all logic lives here in C++. The designer must bind SlotButton, ItemIcon,
 * EmptyIcon, CountLabel, and may bind SlotLabel to show the equipment slot name.
 */
UCLASS(Abstract, Blueprintable)
class ONSET_API UInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Assigns this cell's bag index (INDEX_NONE for equipment cells). */
	void SetSlotIndex(int32 InSlotIndex);

	/** Fills the cell with a bag entry (icon, count, rarity tint). */
	void SetItem(const FOnsetInventoryEntry& InEntry);

	/** Empties the cell (bag placeholder visual). */
	void SetEmpty();

	/** Fills an equipment cell with the equipped row for Slot (empty row = empty). */
	void SetEquipment(EOnsetEquipmentSlot InSlot, FName InRowName);

	/** Empties an equipment cell, showing the slot's name (e.g. "Head"). */
	void SetEquipmentEmpty(EOnsetEquipmentSlot InSlot);

	/** Broadcast when the cell is clicked (SlotIndex, or INDEX_NONE for equipment). */
	UPROPERTY(BlueprintAssignable, Category = "Onset|Inventory")
	FOnsetInventorySlotClicked OnSlotClicked;

protected:
	virtual void NativeConstruct() override;

	/** Hook for Blueprint to style the cell after the logical state was applied. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Onset|Inventory")
	void BP_OnSlotDataChanged();

	/** The designer must provide a button; click wires to OnSlotClicked. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOnsetButtonBase> SlotButton;

	/** Designer icon image; shown when the cell holds an item. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ItemIcon;

	/** Designer placeholder image; shown when the cell is empty. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> EmptyIcon;

	/** Designer count label ("x3"); hidden when Count <= 1. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> CountLabel;

	/** Optional label showing the equipment slot name on loadout cells. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> SlotLabel;

private:
	UFUNCTION()
	void HandleClicked();

	/** Applies the current visual state to the bound widgets and fires BP_OnSlotDataChanged. */
	void ApplyVisualState();

	int32 SlotIndex = INDEX_NONE;
	bool bIsEquipmentCell = false;
	EOnsetEquipmentSlot EquipmentSlot = EOnsetEquipmentSlot::Weapon;
	FOnsetInventoryEntry Entry;
};