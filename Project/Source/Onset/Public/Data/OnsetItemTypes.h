// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "OnsetItemTypes.generated.h"

/** Item family; 1:1 with a DataTable. A row's table determines its category. */
UENUM(BlueprintType)
enum class EOnsetItemCategory : uint8
{
	Equipment	UMETA(DisplayName = "Equipment"),	// DT_Equipment (weapons / armor / accessories)
	QuestItem	UMETA(DisplayName = "Quest Item"),	// DT_QuestItems
	Junk		UMETA(DisplayName = "Junk"),		// DT_Junk
	Scroll		UMETA(DisplayName = "Scroll")		// DT_Scrolls (ability-grant consumables)
};

/** Display/sell-tiering metadata. */
UENUM(BlueprintType)
enum class EOnsetItemRarity : uint8
{
	Common		UMETA(DisplayName = "Common"),
	Uncommon	UMETA(DisplayName = "Uncommon"),
	Rare		UMETA(DisplayName = "Rare"),
	Epic		UMETA(DisplayName = "Epic"),
	Legendary	UMETA(DisplayName = "Legendary")
};

/**
 * Common base for every item row across the category tables. The category
 * tables (DT_Equipment / DT_QuestItems / DT_Junk / DT_Scrolls) each use a
 * derived row struct, so the pipeline can read any item through this base and
 * down-cast for category-specific data.
 */
USTRUCT(BlueprintType)
struct FOnsetItemDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EOnsetItemRarity Rarity = EOnsetItemRarity::Common;

	/** Inclusive minimum level to equip/use; 0 = none. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 LevelRequirement = 0;

	/** 1 = unique (does not stack); >1 = stackable up to this many. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 MaxStackSize = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 SellValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 BuyValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText Description;
};

/** One stacked bag slot: a row in a category table plus a count. */
USTRUCT(BlueprintType)
struct FOnsetInventoryEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	EOnsetItemCategory Category = EOnsetItemCategory::Equipment;

	/** Row name within the category table. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FName RowName;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 Count = 1;
};

/** One row in DT_QuestItems. Quest progress references these via quest data. */
USTRUCT(BlueprintType)
struct FOnsetQuestItemDefinition : public FOnsetItemDefinition
{
	GENERATED_BODY()

	/** Hint shown in tooltips; actual quest wiring arrives with the quest system. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText QuestHint;
};

/** One row in DT_Junk. Vendable filler; keeps an explicit type for future flavor fields. */
USTRUCT(BlueprintType)
struct FOnsetJunkItemDefinition : public FOnsetItemDefinition
{
	GENERATED_BODY()
};

/**
 * One row in DT_Scrolls. Consumables that teach/upgrade abilities — the grant
 * pipeline (learn/upgrade) is a future pass; the data model lives here so loot
 * and quests can drop scrolls now.
 */
USTRUCT(BlueprintType)
struct FOnsetScrollDefinition : public FOnsetItemDefinition
{
	GENERATED_BODY()

	/** DT_Abilities row this scroll grants or upgrades (empty = unassigned). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FDataTableRowHandle GrantedAbility;

	/** 1 = learn; >1 = upgrade target level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 UpgradeLevel = 1;
};