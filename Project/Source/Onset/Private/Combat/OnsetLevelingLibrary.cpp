// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/OnsetLevelingLibrary.h"

#include "Misc/ConfigCacheIni.h"

namespace OnsetLevelingLibraryInternal
{
	float CachedXpBase = -1.0f;
	float CachedXpGrowth = -1.0f;
	float CachedKillsPerLevel = -1.0f;
	float CachedBonusXpPerOverLevel = -1.0f;
	float CachedMaxBonusXp = -1.0f;
	int32 CachedLevelCap = -1;
	int32 CachedGreyThreshold = -1;
	int32 CachedYellowThreshold = -1;
	int32 CachedStatPointsPerLevel = -1;
}

static float ReadConfigFloat(const TCHAR* Key, float Default)
{
	float Value = Default;
	GConfig->GetFloat(TEXT("Onset.Gameplay"), Key, Value, GEngineIni);
	return Value;
}

static int32 ReadConfigInt(const TCHAR* Key, int32 Default)
{
	int32 Value = Default;
	GConfig->GetInt(TEXT("Onset.Gameplay"), Key, Value, GEngineIni);
	return Value;
}

int32 UOnsetLevelingLibrary::GetLevelCap()
{
	if (OnsetLevelingLibraryInternal::CachedLevelCap < 0)
	{
		OnsetLevelingLibraryInternal::CachedLevelCap = FMath::Max(1, ReadConfigInt(TEXT("LevelCap"), 200));
	}
	return OnsetLevelingLibraryInternal::CachedLevelCap;
}

int32 UOnsetLevelingLibrary::GetXPRequired(int32 Level)
{
	const int32 SafeLevel = FMath::Max(1, Level);
	return FMath::Max(1, FMath::RoundToInt(
		GetXpBase() * FMath::Pow(1.0f + GetXpGrowth(), static_cast<float>(SafeLevel - 1))));
}

int32 UOnsetLevelingLibrary::GetEnemyBaseXP(int32 EnemyLevel, int32 XpReward)
{
	if (XpReward > 0)
	{
		return XpReward;
	}
	return FMath::Max(1, FMath::RoundToInt(
		static_cast<float>(GetXPRequired(EnemyLevel)) / GetKillsPerLevel()));
}

float UOnsetLevelingLibrary::GetXPMultiplier(int32 PlayerLevel, int32 EnemyLevel)
{
	const int32 LevelDiff = EnemyLevel - PlayerLevel;
	const int32 GreyThreshold = GetGreyThreshold();
	const int32 YellowThreshold = GetYellowThreshold();

	if (LevelDiff <= -GreyThreshold)
	{
		return 0.0f;
	}

	if (LevelDiff <= -YellowThreshold)
	{
		// Ramp 0% -> 100% between -GreyThreshold and -YellowThreshold.
		const int32 RampSpan = FMath::Max(1, GreyThreshold - YellowThreshold);
		return FMath::Clamp(
			static_cast<float>(LevelDiff + GreyThreshold) / static_cast<float>(RampSpan),
			0.0f, 1.0f);
	}

	if (LevelDiff > 0)
	{
		return 1.0f + FMath::Min(
			static_cast<float>(LevelDiff) * GetBonusXpPerOverLevel(),
			GetMaxBonusXp());
	}

	return 1.0f;
}

int32 UOnsetLevelingLibrary::GetGrantedXP(int32 PlayerLevel, int32 EnemyLevel, int32 XpReward)
{
	return FMath::Max(0, FMath::RoundToInt(
		static_cast<float>(GetEnemyBaseXP(EnemyLevel, XpReward)) * GetXPMultiplier(PlayerLevel, EnemyLevel)));
}

int32 UOnsetLevelingLibrary::GetStatPointsPerLevel()
{
	if (OnsetLevelingLibraryInternal::CachedStatPointsPerLevel < 0)
	{
		OnsetLevelingLibraryInternal::CachedStatPointsPerLevel = FMath::Max(1, ReadConfigInt(TEXT("StatPointsPerLevel"), 3));
	}
	return OnsetLevelingLibraryInternal::CachedStatPointsPerLevel;
}

float UOnsetLevelingLibrary::GetXpBase()
{
	if (OnsetLevelingLibraryInternal::CachedXpBase < 0.0f)
	{
		OnsetLevelingLibraryInternal::CachedXpBase = FMath::Max(1.0f, ReadConfigFloat(TEXT("XpBase"), 50.0f));
	}
	return OnsetLevelingLibraryInternal::CachedXpBase;
}

float UOnsetLevelingLibrary::GetXpGrowth()
{
	if (OnsetLevelingLibraryInternal::CachedXpGrowth < 0.0f)
	{
		OnsetLevelingLibraryInternal::CachedXpGrowth = FMath::Max(0.0f, ReadConfigFloat(TEXT("XpGrowth"), 0.06f));
	}
	return OnsetLevelingLibraryInternal::CachedXpGrowth;
}

float UOnsetLevelingLibrary::GetKillsPerLevel()
{
	if (OnsetLevelingLibraryInternal::CachedKillsPerLevel < 0.0f)
	{
		OnsetLevelingLibraryInternal::CachedKillsPerLevel = FMath::Max(1.0f, ReadConfigFloat(TEXT("KillsPerLevel"), 180.0f));
	}
	return OnsetLevelingLibraryInternal::CachedKillsPerLevel;
}

int32 UOnsetLevelingLibrary::GetGreyThreshold()
{
	if (OnsetLevelingLibraryInternal::CachedGreyThreshold < 0)
	{
		OnsetLevelingLibraryInternal::CachedGreyThreshold = FMath::Max(1, ReadConfigInt(TEXT("GreyThreshold"), 10));
	}
	return OnsetLevelingLibraryInternal::CachedGreyThreshold;
}

int32 UOnsetLevelingLibrary::GetYellowThreshold()
{
	if (OnsetLevelingLibraryInternal::CachedYellowThreshold < 0)
	{
		OnsetLevelingLibraryInternal::CachedYellowThreshold = FMath::Max(1, ReadConfigInt(TEXT("YellowThreshold"), 5));
	}
	return OnsetLevelingLibraryInternal::CachedYellowThreshold;
}

float UOnsetLevelingLibrary::GetBonusXpPerOverLevel()
{
	if (OnsetLevelingLibraryInternal::CachedBonusXpPerOverLevel < 0.0f)
	{
		OnsetLevelingLibraryInternal::CachedBonusXpPerOverLevel = FMath::Max(0.0f, ReadConfigFloat(TEXT("BonusXpPerOverLevel"), 0.05f));
	}
	return OnsetLevelingLibraryInternal::CachedBonusXpPerOverLevel;
}

float UOnsetLevelingLibrary::GetMaxBonusXp()
{
	if (OnsetLevelingLibraryInternal::CachedMaxBonusXp < 0.0f)
	{
		OnsetLevelingLibraryInternal::CachedMaxBonusXp = FMath::Max(0.0f, ReadConfigFloat(TEXT("MaxBonusXp"), 0.50f));
	}
	return OnsetLevelingLibraryInternal::CachedMaxBonusXp;
}