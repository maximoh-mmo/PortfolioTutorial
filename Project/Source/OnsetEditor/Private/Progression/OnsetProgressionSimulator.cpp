// Copyright Epic Games, Inc. All Rights Reserved.

#include "Progression/OnsetProgressionSimulator.h"
#include "Math/UnrealMathUtility.h"

float FOnsetProgressionSimulator::CalculateXPRequired(int32 Level, float XPBase, float XPGrowth, int32 LoopN, float DifficultyScale)
{
	if (Level <= 1)
	{
		return XPBase;
	}
	float Required = XPBase * FMath::Pow(1.0f + XPGrowth, (float)(Level - 1));
	if (LoopN > 0)
	{
		Required *= FMath::Pow(1.0f + DifficultyScale, (float)LoopN);
	}
	return Required;
}

float FOnsetProgressionSimulator::CalculateCumulativeXP(int32 TargetLevel, float XPBase, float XPGrowth)
{
	float Total = 0.0f;
	for (int32 L = 1; L < TargetLevel; ++L)
	{
		Total += CalculateXPRequired(L, XPBase, XPGrowth);
	}
	return Total;
}

float FOnsetProgressionSimulator::CalculateEnemyBaseXP(int32 Level, float XPBase, float XPGrowth, int32 KillsPerLevel, int32 LoopN, float DifficultyScale)
{
	if (KillsPerLevel <= 0)
	{
		return 0.0f;
	}
	return CalculateXPRequired(Level, XPBase, XPGrowth, LoopN, DifficultyScale) / (float)KillsPerLevel;
}

float FOnsetProgressionSimulator::CalculateLevelDiffMultiplier(int32 PlayerLevel, int32 EnemyLevel, int32 GreyThreshold, int32 YellowThreshold, float BonusXPPerOverLevel, float MaxBonusXP)
{
	const int32 Diff = EnemyLevel - PlayerLevel;
	if (Diff <= -GreyThreshold)
	{
		return 0.0f;
	}
	if (Diff <= -YellowThreshold)
	{
		const float Denom = (float)FMath::Max(1, GreyThreshold - YellowThreshold);
		return (float)(Diff + GreyThreshold) / Denom;
	}
	if (Diff <= 0)
	{
		return 1.0f;
	}
	return 1.0f + FMath::Min((float)Diff * BonusXPPerOverLevel, MaxBonusXP);
}

float FOnsetProgressionSimulator::GetWeaponBaseCooldown(EOnsetWeaponArchetype Archetype)
{
	switch (Archetype)
	{
	case EOnsetWeaponArchetype::Dagger:
		return 0.8f;
	case EOnsetWeaponArchetype::Wand:
		return 0.9f;
	case EOnsetWeaponArchetype::Sword:
	case EOnsetWeaponArchetype::Bow:
		return 1.0f;
	case EOnsetWeaponArchetype::Axe:
	case EOnsetWeaponArchetype::Mace:
		return 1.1f;
	case EOnsetWeaponArchetype::Staff:
		return 1.2f;
	case EOnsetWeaponArchetype::Tome:
		return 1.3f;
	case EOnsetWeaponArchetype::Greatsword:
		return 1.8f;
	default:
		return 1.0f;
	}
}

FPlayerCombatSnapshot FOnsetProgressionSimulator::EvaluatePlayer(
	EOnsetCharacterClass CharacterClass,
	const FOnsetCharacterClassInfo* ClassInfo,
	int32 Level,
	const TArray<FOnsetEquipmentDefinition>& EquippedGear,
	const TMap<FName, float>& AllocatedStatPoints,
	float KDefense,
	float KZoneTierScale,
	float HealthPerVitality,
	float BaseCritChance,
	float MaxCritChanceBonus,
	float BaseCritMult,
	float MaxCritMultBonus,
	float KCrit,
	float KCritMultiplier)
{
	FPlayerCombatSnapshot Snapshot;

	const float BaseSTR = ClassInfo ? ClassInfo->BaseStrength : 10.0f;
	const float BaseINT = ClassInfo ? ClassInfo->BaseIntellect : 10.0f;
	const float BaseVIT = ClassInfo ? ClassInfo->BaseVitality : 10.0f;
	const float BaseDEF = ClassInfo ? ClassInfo->BaseDefense : 10.0f;
	const float BaseAGI = ClassInfo ? ClassInfo->BaseAgility : 10.0f;
	const float BaseLUK = ClassInfo ? ClassInfo->BaseLuck : 10.0f;
	const float StartingHP = ClassInfo ? ClassInfo->StartingMaxHealth : 100.0f;

	const float AllocSTR = AllocatedStatPoints.FindRef(TEXT("Strength"));
	const float AllocINT = AllocatedStatPoints.FindRef(TEXT("Intellect"));
	const float AllocVIT = AllocatedStatPoints.FindRef(TEXT("Vitality"));
	const float AllocDEF = AllocatedStatPoints.FindRef(TEXT("Defense"));
	const float AllocAGI = AllocatedStatPoints.FindRef(TEXT("Agility"));
	const float AllocLUK = AllocatedStatPoints.FindRef(TEXT("Luck"));

	float GearSTR = 0.0f;
	float GearINT = 0.0f;
	float GearVIT = 0.0f;
	float GearDEF = 0.0f;
	float GearAGI = 0.0f;
	float GearLUK = 0.0f;
	float WeaponDamage = 0.0f;
	float BlockChance = 0.0f;
	EOnsetWeaponArchetype WeaponArchetype = EOnsetWeaponArchetype::Sword;

	for (const FOnsetEquipmentDefinition& Item : EquippedGear)
	{
		GearSTR += Item.StrengthBonus;
		GearINT += Item.IntellectBonus;
		GearVIT += Item.VitalityBonus;
		GearDEF += Item.DefenseBonus;
		GearAGI += Item.AgilityBonus;
		GearLUK += Item.LuckBonus;

		if (Item.Slot == EOnsetEquipmentSlot::Weapon)
		{
			WeaponDamage += Item.WeaponDamage;
			WeaponArchetype = Item.Archetype;
		}
		else if (Item.Slot == EOnsetEquipmentSlot::Shield)
		{
			BlockChance += Item.BlockChance;
		}
	}

	// Class Mastery bonuses (combat-formulas §11)
	float ClassBonusDamage = 0.0f;
	float ClassBonusCrit = 0.0f;
	float ClassBonusCDR = 0.0f;

	if (CharacterClass == EOnsetCharacterClass::Tank && BlockChance > 0.0f)
	{
		BlockChance += 0.20f;
		GearDEF += 20.0f;
	}
	else if (CharacterClass == EOnsetCharacterClass::Ranged && WeaponArchetype == EOnsetWeaponArchetype::Bow)
	{
		ClassBonusDamage += 0.15f;
		ClassBonusCrit += 0.10f;
	}
	else if (CharacterClass == EOnsetCharacterClass::DPS)
	{
		ClassBonusCDR += 0.15f;
	}

	if (WeaponDamage <= 0.0f)
	{
		WeaponDamage = 20.0f; // Default baseline starter weapon
	}

	Snapshot.TotalSTR = BaseSTR + AllocSTR + GearSTR;
	Snapshot.TotalINT = BaseINT + AllocINT + GearINT;
	Snapshot.TotalVIT = BaseVIT + AllocVIT + GearVIT;
	Snapshot.TotalDEF = BaseDEF + AllocDEF + GearDEF;
	Snapshot.TotalAGI = BaseAGI + AllocAGI + GearAGI;
	Snapshot.TotalLUK = BaseLUK + AllocLUK + GearLUK;
	Snapshot.WeaponBase = WeaponDamage;
	Snapshot.BlockChance = FMath::Clamp(BlockChance, 0.0f, 0.90f);

	Snapshot.MaxHealth = StartingHP + (Snapshot.TotalVIT * HealthPerVitality);

	const float EffectiveKDEF = KDefense * KZoneTierScale;
	Snapshot.DefenseMitigationPct = EffectiveKDEF > 0.0f
		? (Snapshot.TotalDEF / (Snapshot.TotalDEF + EffectiveKDEF))
		: 0.0f;

	const float UnmitigatedRatio = 1.0f - FMath::Clamp(Snapshot.DefenseMitigationPct, 0.0f, 0.95f);
	Snapshot.EffectiveHP = UnmitigatedRatio > 0.001f ? (Snapshot.MaxHealth / UnmitigatedRatio) : Snapshot.MaxHealth * 100.0f;

	const float LukDivChance = Snapshot.TotalLUK + KCrit;
	Snapshot.CritChance = BaseCritChance + (LukDivChance > 0.0f ? ((Snapshot.TotalLUK / LukDivChance) * MaxCritChanceBonus) : 0.0f) + ClassBonusCrit;
	Snapshot.CritChance = FMath::Clamp(Snapshot.CritChance, 0.0f, 0.70f);

	const float LukDivMult = Snapshot.TotalLUK + KCritMultiplier;
	Snapshot.CritMultiplier = BaseCritMult + (LukDivMult > 0.0f ? ((Snapshot.TotalLUK / LukDivMult) * MaxCritMultBonus) : 0.0f);
	Snapshot.CritMultiplier = FMath::Clamp(Snapshot.CritMultiplier, 1.0f, 4.00f);

	// Weapon-scaled physical damage
	Snapshot.RawDamagePerHit = WeaponDamage * (1.0f + (Snapshot.TotalSTR / 100.0f)) * (1.0f + ClassBonusDamage);

	// Haste & Attack speed
	const float BaseCooldown = GetWeaponBaseCooldown(WeaponArchetype);
	const float Haste = Snapshot.TotalAGI / (Snapshot.TotalAGI + 200.0f);
	const float TotalCDR = FMath::Clamp(Haste + ClassBonusCDR, 0.0f, 0.80f);
	const float EffectiveCooldown = FMath::Max(0.20f, BaseCooldown * (1.0f - TotalCDR));
	Snapshot.AttacksPerSecond = 1.0f / EffectiveCooldown;

	const float CritFactor = 1.0f + Snapshot.CritChance * (Snapshot.CritMultiplier - 1.0f);
	Snapshot.NetDamagePerHit = Snapshot.RawDamagePerHit; // Raw unmitigated hit
	Snapshot.DPS = Snapshot.RawDamagePerHit * Snapshot.AttacksPerSecond * CritFactor;

	return Snapshot;
}

FSimulationResult FOnsetProgressionSimulator::SimulateCombat(
	const FPlayerCombatSnapshot& Player,
	const FOnsetEnemyStats& Enemy,
	float KDefense,
	float KZoneTierScale,
	float EnemyRankMultiplier,
	float BlockDamageReduction)
{
	FSimulationResult Result;

	const float EffectiveKDEF = KDefense * KZoneTierScale;

	// 1. Player attacking Enemy
	const float EnemyDEF = Enemy.Defense;
	Result.EnemyMitigationPct = EffectiveKDEF > 0.0f ? (EnemyDEF / (EnemyDEF + EffectiveKDEF)) : 0.0f;
	Result.PlayerNetHitDamage = Player.RawDamagePerHit * (1.0f - Result.EnemyMitigationPct);

	const float PlayerCritFactor = 1.0f + Player.CritChance * (Player.CritMultiplier - 1.0f);
	const float PlayerAvgHitDamage = Result.PlayerNetHitDamage * PlayerCritFactor;
	Result.PlayerDPS = PlayerAvgHitDamage * Player.AttacksPerSecond;

	const float EnemyHP = Enemy.MaxHealth * EnemyRankMultiplier;
	Result.TimeToKillSeconds = Result.PlayerDPS > 0.001f ? (EnemyHP / Result.PlayerDPS) : 999.0f;
	Result.HitsToKill = PlayerAvgHitDamage > 0.001f ? FMath::CeilToInt(EnemyHP / PlayerAvgHitDamage) : 999;

	// 2. Enemy attacking Player
	const float EnemyBaseDamage = Enemy.DamageBase * EnemyRankMultiplier;
	const float EnemyCooldown = GetWeaponBaseCooldown(Enemy.WeaponArchetype);
	const float EnemyAPS = 1.0f / FMath::Max(0.20f, EnemyCooldown);

	const float EnemyCritChance = FMath::Clamp(0.05f + (Enemy.Luck / (Enemy.Luck + 200.0f)) * 0.65f, 0.0f, 0.70f);
	const float EnemyCritMult = FMath::Clamp(1.50f + (Enemy.Luck / (Enemy.Luck + 400.0f)) * 2.50f, 1.0f, 4.00f);
	const float EnemyCritFactor = 1.0f + EnemyCritChance * (EnemyCritMult - 1.0f);

	Result.PlayerMitigationPct = Player.DefenseMitigationPct;
	const float BlockMitigation = 1.0f - (Player.BlockChance * BlockDamageReduction);
	Result.EnemyNetHitDamage = EnemyBaseDamage * (1.0f - Result.PlayerMitigationPct) * BlockMitigation * EnemyCritFactor;
	Result.EnemyDPS = Result.EnemyNetHitDamage * EnemyAPS;

	Result.PlayerSurvivalSeconds = Result.EnemyDPS > 0.001f ? (Player.MaxHealth / Result.EnemyDPS) : 999.0f;
	Result.HitsToKillPlayer = Result.EnemyNetHitDamage > 0.001f ? FMath::CeilToInt(Player.MaxHealth / Result.EnemyNetHitDamage) : 999;

	// 3. Pacing status assessment
	if (Result.TimeToKillSeconds >= 4.0f && Result.TimeToKillSeconds <= 10.0f && Result.PlayerSurvivalSeconds >= 20.0f)
	{
		Result.PacingStatus = TEXT("OPTIMAL (Target 4-10s TTK)");
		Result.PacingColor = FLinearColor(0.12f, 0.85f, 0.25f, 1.0f); // Green
	}
	else if (Result.TimeToKillSeconds > 18.0f)
	{
		Result.PacingStatus = TEXT("CRITICAL (Damage Sponge Grind)");
		Result.PacingColor = FLinearColor(0.95f, 0.15f, 0.15f, 1.0f); // Red
	}
	else if (Result.TimeToKillSeconds < 1.5f)
	{
		Result.PacingStatus = TEXT("CRITICAL (Encounter Trivialized)");
		Result.PacingColor = FLinearColor(0.95f, 0.15f, 0.15f, 1.0f); // Red
	}
	else if (Result.PlayerSurvivalSeconds < 10.0f)
	{
		Result.PacingStatus = TEXT("CRITICAL (Deadly / Player Melts)");
		Result.PacingColor = FLinearColor(0.95f, 0.15f, 0.15f, 1.0f); // Red
	}
	else if (Result.TimeToKillSeconds > 10.0f)
	{
		Result.PacingStatus = TEXT("WARNING (Slow Pace)");
		Result.PacingColor = FLinearColor(0.95f, 0.75f, 0.10f, 1.0f); // Yellow
	}
	else
	{
		Result.PacingStatus = TEXT("WARNING (Fast Burst)");
		Result.PacingColor = FLinearColor(0.95f, 0.75f, 0.10f, 1.0f); // Yellow
	}

	return Result;
}
