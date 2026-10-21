// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/OnsetClassInfoTypes.h"
#include "Data/OnsetEquipmentTypes.h"
#include "OnsetPlayerDataTypes.h"

/** Snapshot of player combat stats evaluated at a specific level, gear loadout, and stat allocation. */
struct FPlayerCombatSnapshot
{
	float MaxHealth = 100.0f;
	float EffectiveHP = 100.0f;
	float RawDamagePerHit = 10.0f;
	float NetDamagePerHit = 10.0f;
	float AttacksPerSecond = 1.0f;
	float DPS = 10.0f;
	float DefenseMitigationPct = 0.0f;
	float CritChance = 0.05f;
	float CritMultiplier = 1.50f;
	float BlockChance = 0.0f;

	// Total primary stats
	float TotalSTR = 10.0f;
	float TotalINT = 10.0f;
	float TotalVIT = 10.0f;
	float TotalDEF = 10.0f;
	float TotalAGI = 10.0f;
	float TotalLUK = 10.0f;
	float WeaponBase = 20.0f;
};

/** Results of two-way simulated combat between player and enemy. */
struct FSimulationResult
{
	float PlayerDPS = 0.0f;
	float EnemyDPS = 0.0f;
	float TimeToKillSeconds = 0.0f;
	int32 HitsToKill = 0;
	float PlayerSurvivalSeconds = 0.0f;
	int32 HitsToKillPlayer = 0;
	float PlayerNetHitDamage = 0.0f;
	float EnemyNetHitDamage = 0.0f;
	float EnemyMitigationPct = 0.0f;
	float PlayerMitigationPct = 0.0f;
	FString PacingStatus; // "OPTIMAL", "SLOW", "FAST", "DEADLY"
	FLinearColor PacingColor = FLinearColor::Green;
};

/**
 * Pure C++ Progression & Combat Simulation engine for Onset.
 * Implements combat formulas codified in Docs/combat-formulas.md.
 */
class FOnsetProgressionSimulator
{
public:
	/** Calculates XP required for a given Level and Rebirth Loop N. */
	static float CalculateXPRequired(int32 Level, float XPBase, float XPGrowth, int32 LoopN = 0, float DifficultyScale = 0.15f);

	/** Calculates cumulative XP required from Level 1 to TargetLevel. */
	static float CalculateCumulativeXP(int32 TargetLevel, float XPBase, float XPGrowth);

	/** Calculates base XP granted by an enemy of a given level. */
	static float CalculateEnemyBaseXP(int32 Level, float XPBase, float XPGrowth, int32 KillsPerLevel, int32 LoopN = 0, float DifficultyScale = 0.15f);

	/** Calculates level difference multiplier (grey / yellow / green mobs). */
	static float CalculateLevelDiffMultiplier(int32 PlayerLevel, int32 EnemyLevel, int32 GreyThreshold = 10, int32 YellowThreshold = 5, float BonusXPPerOverLevel = 0.05f, float MaxBonusXP = 0.50f);

	/** Returns base attack cooldown in seconds for a weapon archetype. */
	static float GetWeaponBaseCooldown(EOnsetWeaponArchetype Archetype);

	/** Evaluates player combat snapshot given class, level, gear, and stat points. */
	static FPlayerCombatSnapshot EvaluatePlayer(
		EOnsetCharacterClass CharacterClass,
		const FOnsetCharacterClassInfo* ClassInfo,
		int32 Level,
		const TArray<FOnsetEquipmentDefinition>& EquippedGear,
		const TMap<FName, float>& AllocatedStatPoints,
		float KDefense = 100.0f,
		float KZoneTierScale = 1.0f,
		float HealthPerVitality = 10.0f,
		float BaseCritChance = 0.05f,
		float MaxCritChanceBonus = 0.65f,
		float BaseCritMult = 1.50f,
		float MaxCritMultBonus = 2.50f,
		float KCrit = 200.0f,
		float KCritMultiplier = 400.0f
	);

	/** Simulates two-way combat between player snapshot and enemy stats. */
	static FSimulationResult SimulateCombat(
		const FPlayerCombatSnapshot& Player,
		const FOnsetEnemyStats& Enemy,
		float KDefense = 100.0f,
		float KZoneTierScale = 1.0f,
		float EnemyRankMultiplier = 1.0f,
		float BlockDamageReduction = 0.50f
	);
};
