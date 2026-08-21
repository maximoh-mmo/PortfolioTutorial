// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OnsetScreenBase.h"
#include "Data/OnsetEquipmentTypes.h"
#include "Data/OnsetItemTypes.h"
#include "InventoryScreen.generated.h"

class UInventorySlotWidget;
class UOnsetInventoryComponent;
class UUniformGridPanel;

/**
 * In-game inventory screen (bag grid + equipped loadout). Derived from in a
 * Widget Blueprint (WBP_InventoryScreen); all logic lives here in C++.
 *
 * Layout is designer-owned: the WBP binds BagGrid (a UniformGridPanel the
 * designer may place inside a vertical ScrollBox) plus one optional slot widget
 * per EOnsetEquipmentSlot, which can be arranged freely (e.g. around a character
 * render). GridRows is exposed (EditAnywhere) so the bag's fixed row count can be
 * tuned per-WBP; columns auto-derive from the inventory's MaxInventorySlots.
 */
UCLASS(Abstract, Blueprintable)
class ONSET_API UInventoryScreen : public UOnsetScreenBase
{
	GENERATED_BODY()

public:
	/** Pops this screen off the Game layer (close button / input toggle). */
	UFUNCTION(BlueprintCallable, Category = "Onset|Inventory")
	void CloseInventory();

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual void NativeDestruct() override;

	/** Resolves the owning pawn's inventory component (null if not possessed). */
	UOnsetInventoryComponent* GetInventoryComponent() const;

	/** Binds/rebinds the inventory-changed delegate of the current pawn's component. */
	void BindToInventory(UOnsetInventoryComponent* InInventory);

	/** Rebuilds the bag slot widgets and refreshes bag + equipment cells. */
	void RebuildSlots();

	/** Pushes the current bag entries into the bag slot widgets. */
	void RefreshBag();

	/** Pushes the current equipped loadout into the bound equipment slot widgets. */
	void RefreshEquipment();

	/** Callback for OnInventoryChanged (authority mutation or OnRep on the client). */
	void HandleInventoryChanged();

	/** Callback for a bag cell click (SlotIndex). Stub for future equip/use/drop. */
	UFUNCTION()
	void HandleSlotClicked(int32 SlotIndex);

	/** Fixed number of rows in the bag grid; columns auto-derive from MaxInventorySlots. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Onset|Inventory|Layout")
	int32 GridRows = 10;

	/** Bag slot widget class to instantiate per cell (override with WBP_InventorySlot). */
	UPROPERTY(EditDefaultsOnly, Category = "Onset|Inventory")
	TSubclassOf<UInventorySlotWidget> InventorySlotWidgetClass;

	/** Designer-bound bag grid (UniformGridPanel, optionally inside a ScrollBox). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UUniformGridPanel> BagGrid;

	// Equipment slots are individually designer-bound so they can be laid out
	// freely (e.g. arranged around a character render). Unbound slots are skipped.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UInventorySlotWidget> WeaponSlot;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UInventorySlotWidget> ShieldSlot;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UInventorySlotWidget> HeadSlot;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UInventorySlotWidget> ChestSlot;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UInventorySlotWidget> HandsSlot;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UInventorySlotWidget> LegsSlot;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UInventorySlotWidget> FeetSlot;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UInventorySlotWidget> AmuletSlot;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UInventorySlotWidget> Ring1Slot;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UInventorySlotWidget> Ring2Slot;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UInventorySlotWidget> TrinketSlot;

	UPROPERTY()
	TObjectPtr<UOnsetInventoryComponent> BoundInventory;

private:
	/** Returns the bound slot widget for Slot, or nullptr if the designer didn't place it. */
	UInventorySlotWidget* GetEquipmentSlotWidget(EOnsetEquipmentSlot Slot) const;

	UPROPERTY()
	TArray<TObjectPtr<UInventorySlotWidget>> BagSlots;
};