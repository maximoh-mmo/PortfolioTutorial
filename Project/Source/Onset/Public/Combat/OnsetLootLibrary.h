// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/OnsetItemTypes.h"
#include "Data/OnsetLootTypes.h"
#include "Engine/DataTable.h"
#include "OnsetLootLibrary.generated.h"

class UDataTable;

/**
 * Loads DT_Loot and rolls loot from it. Mirrors UOnsetEquipmentLibrary's
 * pattern: a config seam ([Onset.Gameplay] LootDataTable) with a cached table.
 */
UCLASS()
class ONSET_API UOnsetLootLibrary : public UObject
{
	GENERATED_BODY()

public:
	/** Path to DT_Loot. Overridable via Onset.Gameplay LootDataTable in DefaultEngine.ini. */
	static FString GetLootTablePath();

	static UDataTable* GetLootTable();

	/** Roll a loot table row into stacked inventory entries (category + row + count).
	 *  Expands sub-tables recursively (cycle-safe), filters by level and zone,
	 *  then rolls each entry's DropChance and quantity range, merging into stacks
	 *  capped by the item's MaxStackSize.
	 */
	static TArray<FOnsetInventoryEntry> RollLoot(const FDataTableRowHandle& Table, const FOnsetLootContext& Context);

	/** Convenience overload that rolls the table's default-assigned row by name. */
	static TArray<FOnsetInventoryEntry> RollLoot(FName TableRowName, const FOnsetLootContext& Context);
};