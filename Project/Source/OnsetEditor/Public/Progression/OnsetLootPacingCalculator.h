// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/OnsetEquipmentTypes.h"
#include "Data/OnsetLootTypes.h"
#include "Engine/DataTable.h"

/** Results of Monte Carlo drop roll simulation. */
struct FMonteCarloResult
{
	int32 TotalKillsSimulated = 10000;
	int32 TotalDropsAcquired = 0;
	float DropsPerKillPct = 0.0f;
	int32 MaxDroughtStreak = 0;
	float AverageKillsBetweenDrops = 0.0f;
	TMap<EOnsetEquipmentSlot, int32> DropsPerSlot;
	bool bMetPacingTarget = false;
	FString SummaryStatus;
};

/**
 * Drop pacing and Monte Carlo simulation calculator for Onset loot tables.
 * Implements models codified in Docs/Inventory/Equipment_Drop_Pacing.md.
 */
class FOnsetLootPacingCalculator
{
public:
	/** Calculates kill budget across a tier level range. Total Kill Budget = DeltaL * KillsPerLevel. */
	static int32 CalculateKillBudget(int32 MinLevel, int32 MaxLevel, int32 KillsPerLevel);

	/** Derives required base equipment drop chance: TargetTotalDrops / TotalKillBudget. */
	static float DeriveRequiredDropChance(int32 TargetItemCount, int32 KillBudget);

	/** Derives subtable inclusion chance: min(1.0, P_equip / P_parent). */
	static float DeriveSubTableInclusionChance(float TargetDropChance, float ParentEntryDropChance);

	/**
	 * Runs Monte Carlo roll validation over SimulatedKills iterations.
	 * Analyzes drop rate, max drought streaks, and slot distribution.
	 */
	static FMonteCarloResult RunSimulation(
		const UDataTable* LootTable,
		FName EntryRowName,
		int32 PlayerLevel,
		int32 SimulatedKills = 10000,
		float TargetDropRate = 0.02f
	);
};
