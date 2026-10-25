// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/OnsetEquipmentTypes.h"
#include "Data/OnsetItemTypes.h"
#include "UOnsetInventoryComponent.generated.h"

/** Broadcast on any inventory or equipment change (authority only). */
DECLARE_MULTICAST_DELEGATE(FOnsetInventoryChanged);

/** Broadcast per-item on every successful AddItem/AddItems (authority only). */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnsetItemAdded, EOnsetItemCategory, FName, int32);

struct FOnsetEquipmentDefinition;

/**
 * Server-authoritative inventory shared by player pawns (bag + equipped loadout)
 * and corpses (loot contents).
 *
 * The bag stores stacked entries (category table + row + count) so any item type
 * — equipment, quest items, junk, scrolls — flows through the same pipeline.
 * Replication is owner-only for player pawns so the owning client can read the
 * bag and loadout; corpses set bReplicateToOwnerOnly = false so every client can
 * see the loot that was rolled server-side at spawn.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ONSET_API UOnsetInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOnsetInventoryComponent();

	// --- Bag (stacked entries: category table + row + count) ---

	/** Adds Count of a row, merging into an existing stack up to MaxStackSize. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddItem(EOnsetItemCategory Category, FName RowName, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddItems(const TArray<FOnsetInventoryEntry>& Entries);

	/** Removes Count of a row. Returns false if insufficient quantity present. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(EOnsetItemCategory Category, FName RowName, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RemoveAllOfItem(EOnsetItemCategory Category, FName RowName);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearItems();

	/** Bulk-replace the bag (used to stamp server-rolled loot onto a corpse). */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetItems(const TArray<FOnsetInventoryEntry>& Entries);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasItem(EOnsetItemCategory Category, FName RowName) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount(EOnsetItemCategory Category, FName RowName) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FOnsetInventoryEntry>& GetItems() const { return Items; }

	/** Total instances across all stacks. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetTotalItemCount() const;

	// --- Equipped loadout ---

	/** Equips RowName into Slot (empty row name removes the slot's item). */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void EquipItem(EOnsetEquipmentSlot Slot, FName RowName);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UnequipSlot(EOnsetEquipmentSlot Slot);

	/**
	 * Moves one instance of RowName from the bag into its slot (resolved from
	 * DT_Equipment). Replaces whatever was in that slot. Returns false if the
	 * item is not in the bag or does not resolve.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool EquipFromInventory(FName RowName);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FName GetEquippedRow(EOnsetEquipmentSlot Slot) const;

	/** Resolves the equipped row's definition (nullptr if empty/unknown). */
	const FOnsetEquipmentDefinition* GetEquippedItem(EOnsetEquipmentSlot Slot) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	TMap<EOnsetEquipmentSlot, FName> GetEquippedMap() const;

	/** Equips item with specific instance state into Slot. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void EquipItemWithInstance(EOnsetEquipmentSlot Slot, FName RowName, const FOnsetEquipmentInstanceData& InstanceData);

	/** Moves item at BagIndex into its designated equipment slot, preserving its instance data. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool EquipFromInventorySlot(int32 BagIndex);

	/** Adds an equipment item with specific instance data to the bag. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddEquipmentInstance(FName RowName, const FOnsetEquipmentInstanceData& InstanceData);

	/** Returns entry at BagIndex, or false if index is out of bounds. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool GetItemAtSlot(int32 SlotIndex, FOnsetInventoryEntry& OutEntry) const;

	/** Const pointer to equipped entry for Slot. */
	const FOnsetEquippedEntry* GetEquippedEntry(EOnsetEquipmentSlot Slot) const;

	/** All equipped entries. */
	const TArray<FOnsetEquippedEntry>& GetEquippedEntries() const { return EquippedEntries; }

	/** Retrieves instance data for equipped slot. Returns true if slot has an item. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool GetEquippedInstanceData(EOnsetEquipmentSlot Slot, FOnsetEquipmentInstanceData& OutData) const;

	// --- Durability & Repair ---

	/** Degrades the equipped weapon by Amount. Server only. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Durability")
	void DegradeEquippedWeapon(float Amount = 1.0f);

	/** Degrades all equipped armor pieces (Shield, Head, Chest, Hands, Legs, Feet) by Amount. Server only. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Durability")
	void DegradeEquippedArmor(float Amount = 1.0f);

	/** Deducts durability from all equipped items on player death (-Percent%). Server only. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Durability")
	void ApplyDurabilityLossOnDeath(float Percent = 15.0f);

	/** Repairs equipped item in Slot using CompositeScrap materials from bag. Server only. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Repair")
	bool RepairEquippedItem(EOnsetEquipmentSlot Slot);

	/** Repairs all equipped damaged items using CompositeScrap materials from bag. Server only. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Repair")
	bool RepairAllEquipped();

	/** Repairs bag item at BagSlotIndex using CompositeScrap materials from bag. Server only. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Repair")
	bool RepairBagItem(int32 BagSlotIndex);

	// --- Salvage & Infusion ---

	/** Dismantles equipment item at BagSlotIndex into scrap/alloys/materials. Server only. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Salvage")
	bool SalvageItem(int32 BagSlotIndex, TArray<FOnsetInventoryEntry>& OutYield);

	/** Infuses Target equipment item with Catalyst duplicate, progressing stat grades. Server only. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Infusion")
	bool InfuseItem(int32 TargetSlotIndex, int32 CatalystSlotIndex, bool& bOutGradeUp, bool& bOutCriticalProc);

	// --- Persistence ---

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FString SerializeEquipmentJSON() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void DeserializeEquipmentJSON(const FString& JSON);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FString SerializeInventoryJSON() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void DeserializeInventoryJSON(const FString& JSON);

	/** Fired on any authority-side mutation (items or equipment). */
	FOnsetInventoryChanged OnInventoryChanged;

	/** Fired per-item on successful AddItem/AddItems grants (authority only). */
	FOnsetItemAdded OnItemAdded;

	/** Maximum number of distinct item rows the inventory can hold (e.g. 80). */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 MaxInventorySlots = 80;

	/** Sets replication scope; must be set before the owning actor registers replication. */
	void SetReplicateToOwnerOnly(bool bInOwnerOnly) { bReplicateToOwnerOnly = bInOwnerOnly; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	/** Bag contents as stacked entries. */
	UPROPERTY(ReplicatedUsing = OnRep_Items, BlueprintReadOnly, Category = "Inventory")
	TArray<FOnsetInventoryEntry> Items;

	/** Equipped loadout as slot entries (TMap replication is unsupported). */
	UPROPERTY(ReplicatedUsing = OnRep_Equipped, BlueprintReadOnly, Category = "Inventory")
	TArray<FOnsetEquippedEntry> EquippedEntries;

	/** Replication scope; must be set before the owning actor registers replication. */
	UPROPERTY()
	bool bReplicateToOwnerOnly = true;

	UFUNCTION()
	void OnRep_Items();

	UFUNCTION()
	void OnRep_Equipped();
};