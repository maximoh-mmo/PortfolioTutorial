// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/OnsetEquipmentTypes.h"
#include "OnsetPlayerDataTypes.h"
#include "UObject/Object.h"
#include "OnsetEquipmentLibrary.generated.h"

class UDataTable;
struct FOnsetCharacterClassInfo;
struct FOnsetEnemyStats;

/**
 * Static registry for DT_Equipment + DT_ClassInfo + DT_EnemyStats.
 *
 * Loads the equipment DataTable lazily and resolves item rows by RowName. Also reads
 * the class DataTable for base stats. Everything falls back to baked code defaults
 * (FOnsetClassBaseStats / MakeDefaultWeaponForClass) so the game runs without either
 * asset; authoring the tables upgrades the numbers without touching code.
 */
UCLASS()
class ONSET_API UOnsetEquipmentLibrary : public UObject
{
	GENERATED_BODY()

public:
	/** Path to DT_Equipment. Overridable via Onset.Gameplay EquipmentDataTable in DefaultEngine.ini. */
	static FString GetEquipmentTablePath();

	/** Returns the loaded equipment DataTable, or null if missing. Loads+caches on first call. */
	static UDataTable* GetEquipmentTable();

	/** Returns the definition for RowName, or null if the row/table is missing. */
	static const FOnsetEquipmentDefinition* GetDefinition(FName RowName);

	/** Path to DT_ClassInfo. Overridable via Onset.Gameplay ClassDataTable in DefaultEngine.ini. */
	static FString GetClassDataTablePath();

	/** Returns the class row for Class, or null if the table/row is missing. */
	static const FOnsetCharacterClassInfo* GetClassInfo(EOnsetCharacterClass Class);

	/** Base stats for Class from DT_ClassInfo, falling back to the baked defaults. */
	static FOnsetClassBaseStats GetClassBaseStats(EOnsetCharacterClass Class);

	/**
	 * Baked fallback weapon for Class (used when nothing is equipped or the row can't
	 * resolve). Tank/DPS get a Sword, Ranged a Bow, Support a Staff with a weaker
	 * basic attack (casters lean on Skill-scaled abilities).
	 */
	static FOnsetEquipmentDefinition MakeDefaultWeaponForClass(EOnsetCharacterClass Class);

	/** Flat fallback weapon base used by NPCs and bare-handed characters. */
	static float GetDefaultWeaponDamage();

	/**
	 * Base cooldown (seconds) of a weapon archetype's basic attack:
	 * Dagger/Wand fastest, Greatsword slowest. Drives the basic-attack cooldown so
	 * the attack rate follows the equipped weapon (combat-formulas §12).
	 */
	static float GetArchetypeBaseCooldown(EOnsetWeaponArchetype Archetype);

	/** True for melee weapon archetypes (the dual-wield CDR source). */
	static bool IsMeleeArchetype(EOnsetWeaponArchetype Archetype);

	// --- Class-mastery tuning constants (combat-formulas §11 / §14) ---

	/** Tank: bonus shield block chance when wielding a shield. */
	static constexpr float GetTankMasteryBlock() { return 0.20f; }

	/** Tank: flat shield Defense bonus. */
	static constexpr float GetTankMasteryDefense() { return 20.0f; }

	/** Dual-wield base CDR for a melee weapon with an empty off-hand. */
	static constexpr float GetDualWieldBaseCDR() { return 0.20f; }

	/** MeleeDPS (DPS class) extra dual-wield CDR on top of the base. */
	static constexpr float GetMeleeDPSBonusCDR() { return 0.15f; }

	/** Ranged: bonus crit chance while wielding a Bow. */
	static constexpr float GetBowMasteryCritBonus() { return 0.10f; }

	/** Ranged: bonus weapon damage while wielding a Bow. */
	static constexpr float GetBowMasteryDamageBonus() { return 0.15f; }

	/** Support: buff/debuff magnitude multiplier bonus. */
	static constexpr float GetSupportMasteryPotencyBonus() { return 0.20f; }

	/** Enemy difficulty growth per tier: stats x (1 + d)^Tier (d = 15%, combat-formulas §10). */
	static constexpr float GetEnemyDifficultyGrowth() { return 0.15f; }

	/** Player prestige outgoing multiplier per level: x (1 + r)^N (r = 10%). */
	static constexpr float GetPrestigeGrowth() { return 0.10f; }

	/** (1 + d)^Tier multiplier for enemy stats at a given difficulty tier. */
	static float GetEnemyDifficultyMultiplier(int32 Tier);

	/** (1 + r)^N multiplier for a given prestige level N. */
	static float GetPrestigeMultiplier(int32 PrestigeLevel);

	/**
	 * Zone-tier K scale for K_DEF / K_elem (combat-formulas §14). Reads
	 * Onset.Gameplay KZoneTierScale from DefaultEngine.ini, default 1.0. No zone-tier
	 * system exists yet; this is the tuning seam for when it does.
	 */
	static float GetZoneTierKScale();

	/** Path to DT_EnemyStats. Overridable via Onset.Gameplay EnemyStatsDataTable in DefaultEngine.ini. */
	static FString GetEnemyStatsTablePath();

	/** Returns the loaded enemy-stats DataTable, or null if missing. Loads+caches on first call. */
	static UDataTable* GetEnemyStatsTable();

	/** Returns the stats row for RowName, or null if the row/table is missing. */
	static const FOnsetEnemyStats* GetEnemyStats(FName RowName);
};