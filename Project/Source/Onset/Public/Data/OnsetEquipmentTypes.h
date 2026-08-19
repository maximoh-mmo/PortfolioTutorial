// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Data/OnsetAbilityTypes.h"
#include "Data/OnsetItemTypes.h"
#include "OnsetEquipmentTypes.generated.h"

class UVisualProfile;
class UAIProfile;
class UPerceptionProfile;

/** Which equipment slot an item occupies. */
UENUM(BlueprintType)
enum class EOnsetEquipmentSlot : uint8
{
	Weapon		UMETA(DisplayName = "Weapon"),
	Shield		UMETA(DisplayName = "Shield"),
	Head		UMETA(DisplayName = "Head"),
	Chest		UMETA(DisplayName = "Chest"),
	Hands		UMETA(DisplayName = "Hands"),
	Legs		UMETA(DisplayName = "Legs"),
	Feet		UMETA(DisplayName = "Feet"),
	Amulet		UMETA(DisplayName = "Amulet"),
	Ring1		UMETA(DisplayName = "Ring 1"),
	Ring2		UMETA(DisplayName = "Ring 2"),
	Trinket		UMETA(DisplayName = "Trinket")
};

/** One equipped slot entry. Replicated as an array (TMap replication is unsupported). */
USTRUCT(BlueprintType)
struct FOnsetEquippedEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	EOnsetEquipmentSlot Slot = EOnsetEquipmentSlot::Weapon;

	/** DT_Equipment row ID. Empty = nothing equipped in this slot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FName RowName;
};

/**
 * Weapon archetype. Drives the base-cooldown table in Phase 3 (combat-formulas §9):
 * Dagger 0.8 / Wand 0.9 / Sword·Bow 1.0 / Axe·Mace 1.1 / Staff 1.2 / Tome 1.3 / Greatsword 1.8.
 */
UENUM(BlueprintType)
enum class EOnsetWeaponArchetype : uint8
{
	Sword		UMETA(DisplayName = "Sword"),
	Dagger		UMETA(DisplayName = "Dagger"),
	Axe			UMETA(DisplayName = "Axe"),
	Mace		UMETA(DisplayName = "Mace"),
	Greatsword	UMETA(DisplayName = "Greatsword"),
	Staff		UMETA(DisplayName = "Staff"),
	Wand		UMETA(DisplayName = "Wand"),
	Tome		UMETA(DisplayName = "Tome"),
	Bow			UMETA(DisplayName = "Bow")
};

/**
 * One row in DT_Equipment. RowName is the stable item ID (e.g. "IronSword").
 *
 * - Weapon slot: WeaponDamage is the WeaponBase used by weapon-scaled abilities
 *   (Raw = WeaponDamage x (1 + STR/100)); DamageElement is the basic attack's element.
 * - Shield slot: BlockChance feeds the pre-mitigation block stage; DefenseBonus is a
 *   flat DEF bonus folded into RecalculateDerivedStats.
 * - Armor slots (Head/Chest/Hands/Legs/Feet): DefenseBonus contributes flat DEF.
 * - Accessory slots (Amulet/Ring1/Ring2/Trinket): stat bonuses.
 * - Stat bonuses apply to any slot and are summed into the character's stats.
 */
USTRUCT(BlueprintType)
struct FOnsetEquipmentDefinition : public FOnsetItemDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	EOnsetEquipmentSlot Slot = EOnsetEquipmentSlot::Weapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	EOnsetWeaponArchetype Archetype = EOnsetWeaponArchetype::Sword;

	/** WeaponBase for weapon-scaled abilities (Weapon slot only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	float WeaponDamage = 0.0f;

	/** Basic attack damage element (Weapon slot only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	EOnsetDamageElement DamageElement = EOnsetDamageElement::Physical;

	/** Chance a shield blocks a hit; 0 = no shield (Shield slot only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	float BlockChance = 0.0f;

	/** Flat DEF bonus granted while equipped (Shield + armor slots). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	float DefenseBonus = 0.0f;

	// --- Flat stat bonuses (either slot) ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	float StrengthBonus = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	float IntellectBonus = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	float VitalityBonus = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	float AgilityBonus = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	float LuckBonus = 0.0f;
};

/**
 * Class base stat block used by RecalculateDerivedStats when the DT_ClassInfo row is
 * missing or a class isn't represented there yet.
 */
USTRUCT(BlueprintType)
struct FOnsetClassBaseStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class")
	float Strength = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class")
	float Intellect = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class")
	float Vitality = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class")
	float Defense = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class")
	float Agility = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class")
	float Luck = 10.0f;
};

/**
 * One row in DT_EnemyStats. RowName is the stable enemy type ID (e.g. "GoblinMelee").
 *
 * The values are the baseline at difficulty tier 0; AOnsetEnemy::ApplyEnemyStats
 * scales MaxHealth and DamageBase by (1 + d)^Tier (d = 15%, combat-formulas §10).
 * ElementAffinity grants the enemy's Element.* tag so the type chart (Phase 4) applies.
 */
USTRUCT(BlueprintType)
struct FOnsetEnemyStats : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	FText DisplayName;

	/** Authored enemy level (1-200). Drives the XP LevelDiff multiplier and the
	 *  level-derived XP fallback (combat-formulas §12). Not scaled by tier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	int32 Level = 1;

	/** Explicit XP reward on kill. 0 = derive from Level via XPRequired(Level)/KillsPerLevel.
	 *  Set > 0 to override (e.g. bosses give more, summoned minions give 0). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	int32 XpReward = 0;

	/** Max health at tier 0; scaled by (1 + d)^Tier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float MaxHealth = 100.0f;

	/** Basic-attack WeaponBase at tier 0; scaled by (1 + d)^Tier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float DamageBase = 25.0f;

	/** Physical mitigation stat (DEF/(DEF + K_DEF)). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float Defense = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float ResistanceFire = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float ResistanceIce = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float ResistanceLightning = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float ResistancePoison = 0.0f;

	/** Critical strike input. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float Luck = 10.0f;

	/** Drives the enemy's basic-attack cooldown (Phase 3 archetype table). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	EOnsetWeaponArchetype WeaponArchetype = EOnsetWeaponArchetype::Sword;

	/** Target affinity for the type chart; Physical (= no Element.* tag) is neutral. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	EOnsetDamageElement ElementAffinity = EOnsetDamageElement::Physical;

	/** DT_Loot row rolled on death; empty = no drops. Shared across enemy types. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (RowType = "/Script/Onset.OnsetLootTableRow"))
	FDataTableRowHandle LootTable;

	/**
	 * Visual profile (mesh, corpse mesh, anim BP, material). When set, the row is
	 * a complete enemy definition: spawners without an explicit FSpawnConfig
	 * profile fall back to these. Empty = keep the spawner's config value.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	TObjectPtr<UVisualProfile> VisualProfile;

	/** AI profile (state tree, aggression, ranges). Spawner config overrides this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	TObjectPtr<UAIProfile> AIProfile;

	/** Perception profile (sight range/angle, hearing). Spawner config overrides this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	TObjectPtr<UPerceptionProfile> PerceptionProfile;
};