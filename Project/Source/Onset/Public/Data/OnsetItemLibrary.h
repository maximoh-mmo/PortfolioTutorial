// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/OnsetItemTypes.h"
#include "UObject/Object.h"
#include "OnsetItemLibrary.generated.h"

class UDataTable;

/**
 * Registry for the per-category item tables (DT_Equipment / DT_QuestItems /
 * DT_Junk / DT_Scrolls). Category is 1:1 with a table; the table path for each
 * is overridable via Onset.Gameplay <Category>DataTable seams in DefaultEngine.ini.
 */
UCLASS()
class ONSET_API UOnsetItemLibrary : public UObject
{
	GENERATED_BODY()

public:
	/** Table path for a category, from the Onset.Gameplay ini seam (baked default fallback). */
	static FString GetTablePath(EOnsetItemCategory Category);

	/** Loads (and caches) the table for a category; null if missing. */
	static UDataTable* GetTable(EOnsetItemCategory Category);

	/** Reads an item row across the category tables as the common base; null if missing. */
	static const FOnsetItemDefinition* GetItemDefinition(EOnsetItemCategory Category, FName RowName);

	/** The category owning Table, or the default category if Table is unknown/null. */
	static EOnsetItemCategory GetCategoryForTable(const UDataTable* Table);

	/** Stack cap for a row; 1 when the row/table is missing. */
	static int32 GetMaxStackSize(EOnsetItemCategory Category, FName RowName);

	/**
	 * Adds Count of (Category, RowName) to Bag, merging into an existing entry
	 * of the same item up to its MaxStackSize (splitting into further entries
	 * when a stack overflows). No owner/authority checks — pure container math.
	 */
	static void AddStacked(TArray<FOnsetInventoryEntry>& Bag, EOnsetItemCategory Category, FName RowName, int32 Count);
};