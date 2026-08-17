// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "OnsetLootTypes.generated.h"

/** Context passed to a loot roll. Level is the enemy's difficulty tier in this project. */
USTRUCT(BlueprintType)
struct FOnsetLootContext
{
	GENERATED_BODY()

	/** Source level/tier used for MinLevel/MaxLevel gating. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	int32 Level = 0;

	/** Spawner-assigned area tag used for RequiredZoneTag gating. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	FGameplayTag ZoneTag;
};

/**
 * One item instance in a loot table. Items that fail the level or zone checks
 * are filtered out of the roll; survivors roll DropChance and drop a uniform
 * quantity in [MinQty, MaxQty].
 */
USTRUCT(BlueprintType)
struct FOnsetLootEntry
{
	GENERATED_BODY()

	/** Row in any category item table to drop (equipment / quest / junk / scroll). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	FDataTableRowHandle Item;

	/** 0..1 chance this entry drops per roll. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DropChance = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "1"))
	int32 MinQty = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "1"))
	int32 MaxQty = 1;

	/** Only rolled when the context ZoneTag matches; empty = any zone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	FGameplayTag RequiredZoneTag;

	/** Inclusive level bounds; both 0 = unbounded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "0"))
	int32 MinLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "0"))
	int32 MaxLevel = 0;
};

/** A reference to another DT_Loot row, pulled into this table when rolled. */
USTRUCT(BlueprintType)
struct FOnsetLootSubTableRef
{
	GENERATED_BODY()

	/** Row in DT_Loot to expand in-place. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (RowType = "/Script/Onset.OnsetLootTableRow"))
	FDataTableRowHandle Table;

	/** 0..1 chance the whole sub-table participates in the roll; 1 = always. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float InclusionChance = 1.0f;
};

/**
 * One named, reusable loot table in DT_Loot. A table is its own entries plus the
 * (recursively expanded) sub-tables it references, so shared "common" tables can
 * be composed by many enemy types and augmented per type/area/level.
 */
USTRUCT(BlueprintType)
struct FOnsetLootTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	TArray<FOnsetLootEntry> Entries;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	TArray<FOnsetLootSubTableRef> SubTables;
};