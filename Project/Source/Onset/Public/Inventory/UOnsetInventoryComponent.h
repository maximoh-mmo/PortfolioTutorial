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