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
	Scroll		UMETA(DisplayName = "Scroll"),		// DT_Scrolls (ability-grant consumables)
	Material	UMETA(DisplayName = "Material")		// DT_Materials (salvage & crafting components)
};

/** Quality grade of rolled equipment stats (D = Surplus/Defective, S = Apex/Masterwork). */
UENUM(BlueprintType)
enum class EOnsetQualityGrade : uint8
{
	GradeD		UMETA(DisplayName = "Grade D (Surplus)"),
	GradeC		UMETA(DisplayName = "Grade C (Standard)"),
	GradeB		UMETA(DisplayName = "Grade B (Refined)"),
	GradeA		UMETA(DisplayName = "Grade A (Overclocked)"),
	GradeS		UMETA(DisplayName = "Grade S (Apex)")
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

/**
 * Per-instance mutable state for equipment items.
 * Carried on FOnsetInventoryEntry (bag) and FOnsetEquippedEntry (equipped loadout).
 */
USTRUCT(BlueprintType)
struct FOnsetEquipmentInstanceData
{
	GENERATED_BODY()

	/** Current item durability (0 = Malfunctioning). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	float CurrentDurability = 100.0f;

	/** Maximum durability cap for this item. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	float MaxDurability = 100.0f;

	/** Quality grade determining multiplier on primary combat stats (Damage / Defense). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	EOnsetQualityGrade PrimaryStatGrade = EOnsetQualityGrade::GradeC;

	/** Quality grade determining multiplier on secondary attribute bonuses (STR, INT, VIT, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	EOnsetQualityGrade SecondaryStatGrade = EOnsetQualityGrade::GradeC;

	/** Infusion progress towards the next grade promotion (0.0 to 1.0). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	float InfusionProgress = 0.0f;

	/** Number of successful infusion promotions this item has undergone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	int32 InfusionLevel = 0;

	/** Whether the item is malfunctioning (0 durability: 50% stat penalty). */
	bool IsMalfunctioning() const { return CurrentDurability <= 0.0f; }

	/** Durability percentage (0.0 to 1.0). */
	float GetDurabilityPercent() const
	{
		return MaxDurability > 0.0f ? FMath::Clamp(CurrentDurability / MaxDurability, 0.0f, 1.0f) : 0.0f;
	}
};

/** One stacked bag slot: a row in a category table plus a count and instance data for equipment. */
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

	/** Instance-specific state (durability, stat grades, infusion). Used when Category == Equipment. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FOnsetEquipmentInstanceData InstanceData;
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

/** Material tier for salvage, crafting, and repairs. */
UENUM(BlueprintType)
enum class EOnsetMaterialTier : uint8
{
	Scrap		UMETA(DisplayName = "Composite Scrap"),		// Low-tier scrap repair sink
	Alloy		UMETA(DisplayName = "Reinforced Alloy"),	// Mid-tier alloy
	Filament	UMETA(DisplayName = "Nanite Filament"),		// High-tier filament
	Core		UMETA(DisplayName = "Quantum Core")			// Top-tier core
};

/** One row in DT_Materials. Salvage yields, crafting components, repair catalysts. */
USTRUCT(BlueprintType)
struct FOnsetMaterialDefinition : public FOnsetItemDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
	EOnsetMaterialTier MaterialTier = EOnsetMaterialTier::Scrap;
};