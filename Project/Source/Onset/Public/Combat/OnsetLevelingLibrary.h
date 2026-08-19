// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "OnsetLevelingLibrary.generated.h"

/**
 * Static formulas for experience and leveling (combat-formulas §12).
 *
 * Curve: XPRequired(Level) = XPBase * (1 + XPGrowth)^(Level-1). Enemy base XP is built
 * from the same curve (XPRequired / KillsPerLevel) so kills-per-level stays flat across
 * the whole range; an authored XpReward on an enemy row overrides it (boss = more,
 * summoned minion = 0). A grey/yellow/green level-diff multiplier scales the reward.
 *
 * All constants live in Onset.Gameplay (DefaultEngine.ini) and are cached on first read,
 * mirroring the KZoneTierScale seam in UOnsetEquipmentLibrary.
 */
UCLASS()
class ONSET_API UOnsetLevelingLibrary : public UObject
{
	GENERATED_BODY()

public:
	/** Max player level per loop. Reads Onset.Gameplay LevelCap (default 200). */
	static int32 GetLevelCap();

	/** XP required to advance from Level to Level+1 (Loop 0). XPBase * (1+XPGrowth)^(Level-1). */
	static int32 GetXPRequired(int32 Level);

	/**
	 * Base XP a kill at EnemyLevel grants before the level-diff multiplier.
	 * XpReward > 0 is used verbatim (boss = more, summoned minion = 0); XpReward == 0
	 * falls back to XPRequired(EnemyLevel) / KillsPerLevel so pacing stays flat.
	 */
	static int32 GetEnemyBaseXP(int32 EnemyLevel, int32 XpReward);

	/**
	 * Grey/yellow/green level-diff multiplier (0..1, then 1..1+MaxBonusXP).
	 * LevelDiff = EnemyLevel - PlayerLevel.
	 */
	static float GetXPMultiplier(int32 PlayerLevel, int32 EnemyLevel);

	/** Round(GetEnemyBaseXP * GetXPMultiplier). The XP actually granted. */
	static int32 GetGrantedXP(int32 PlayerLevel, int32 EnemyLevel, int32 XpReward);

	/** Stat points awarded per level-up. Reads Onset.Gameplay StatPointsPerLevel (default 3). */
	static int32 GetStatPointsPerLevel();

	/** XpBase per-level curve magnitude (default 50). */
	static float GetXpBase();

	/** XPGrowth compounding rate (default 6%). */
	static float GetXpGrowth();

	/** Primary pacing knob: on-level kills to gain a level (default 180). */
	static float GetKillsPerLevel();

	/** Grey threshold: LevelDiff <= -GreyThreshold grants 0 XP (default 10). */
	static int32 GetGreyThreshold();

	/** Yellow threshold: ramp reaches 100% at LevelDiff = -YellowThreshold (default 5). */
	static int32 GetYellowThreshold();

	/** Bonus XP % per level the enemy outlevels the player (default 5%). */
	static float GetBonusXpPerOverLevel();

	/** Cap on the over-level bonus (default 50%). */
	static float GetMaxBonusXp();
};