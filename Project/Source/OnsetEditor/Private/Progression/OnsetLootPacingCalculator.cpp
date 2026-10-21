// Copyright Epic Games, Inc. All Rights Reserved.

#include "Progression/OnsetLootPacingCalculator.h"
#include "Combat/OnsetEquipmentLibrary.h"
#include "Combat/OnsetLootLibrary.h"
#include "Data/OnsetItemTypes.h"
#include "Math/UnrealMathUtility.h"

int32 FOnsetLootPacingCalculator::CalculateKillBudget(int32 MinLevel, int32 MaxLevel, int32 KillsPerLevel)
{
	const int32 LevelSpan = FMath::Max(1, MaxLevel - MinLevel + 1);
	return LevelSpan * FMath::Max(1, KillsPerLevel);
}

float FOnsetLootPacingCalculator::DeriveRequiredDropChance(int32 TargetItemCount, int32 KillBudget)
{
	if (KillBudget <= 0)
	{
		return 0.0f;
	}
	return (float)TargetItemCount / (float)KillBudget;
}

float FOnsetLootPacingCalculator::DeriveSubTableInclusionChance(float TargetDropChance, float ParentEntryDropChance)
{
	if (ParentEntryDropChance <= 0.0f)
	{
		return 1.0f;
	}
	return FMath::Clamp(TargetDropChance / ParentEntryDropChance, 0.0f, 1.0f);
}

FMonteCarloResult FOnsetLootPacingCalculator::RunSimulation(
	const UDataTable* LootTable,
	FName EntryRowName,
	int32 PlayerLevel,
	int32 SimulatedKills,
	float TargetDropRate)
{
	FMonteCarloResult Result;
	Result.TotalKillsSimulated = FMath::Max(1, SimulatedKills);

	int32 CurrentDrought = 0;
	bool bUsedActualTable = false;

	if (LootTable && !EntryRowName.IsNone())
	{
		FDataTableRowHandle Handle;
		Handle.DataTable = const_cast<UDataTable*>(LootTable);
		Handle.RowName = EntryRowName;

		FOnsetLootContext Context;
		Context.Level = PlayerLevel;

		if (LootTable->FindRow<FOnsetLootTableRow>(EntryRowName, nullptr))
		{
			bUsedActualTable = true;
			for (int32 i = 0; i < Result.TotalKillsSimulated; ++i)
			{
				TArray<FOnsetInventoryEntry> Rolled = UOnsetLootLibrary::RollLoot(Handle, Context);
				int32 EquipCount = 0;
				for (const FOnsetInventoryEntry& Entry : Rolled)
				{
					if (Entry.Category == EOnsetItemCategory::Equipment)
					{
						EquipCount += Entry.Count;
						if (const FOnsetEquipmentDefinition* Def = UOnsetEquipmentLibrary::GetDefinition(Entry.RowName))
						{
							Result.DropsPerSlot.FindOrAdd(Def->Slot, 0) += Entry.Count;
						}
						else
						{
							Result.DropsPerSlot.FindOrAdd(EOnsetEquipmentSlot::Weapon, 0) += Entry.Count;
						}
					}
				}

				if (EquipCount > 0)
				{
					Result.TotalDropsAcquired += EquipCount;
					CurrentDrought = 0;
				}
				else
				{
					CurrentDrought++;
					Result.MaxDroughtStreak = FMath::Max(Result.MaxDroughtStreak, CurrentDrought);
				}
			}
		}
	}

	if (!bUsedActualTable)
	{
		const float EffectiveRate = TargetDropRate > 0.0f ? TargetDropRate : 0.0204f;
		for (int32 i = 0; i < Result.TotalKillsSimulated; ++i)
		{
			if (FMath::FRand() <= EffectiveRate)
			{
				Result.TotalDropsAcquired++;
				CurrentDrought = 0;
				const uint8 SlotIdx = (uint8)FMath::RandRange(0, 10);
				Result.DropsPerSlot.FindOrAdd(static_cast<EOnsetEquipmentSlot>(SlotIdx), 0)++;
			}
			else
			{
				CurrentDrought++;
				Result.MaxDroughtStreak = FMath::Max(Result.MaxDroughtStreak, CurrentDrought);
			}
		}
	}

	Result.DropsPerKillPct = ((float)Result.TotalDropsAcquired / (float)Result.TotalKillsSimulated) * 100.0f;
	Result.AverageKillsBetweenDrops = Result.TotalDropsAcquired > 0 ? ((float)Result.TotalKillsSimulated / (float)Result.TotalDropsAcquired) : (float)Result.TotalKillsSimulated;

	const float ExpectedRatePct = (TargetDropRate > 0.0f ? TargetDropRate : 0.0204f) * 100.0f;
	const float LowerBound = ExpectedRatePct * 0.70f;
	const float UpperBound = ExpectedRatePct * 1.30f;

	Result.bMetPacingTarget = (Result.DropsPerKillPct >= LowerBound && Result.DropsPerKillPct <= UpperBound);
	if (Result.bMetPacingTarget)
	{
		Result.SummaryStatus = FString::Printf(TEXT("✅ Balanced Pacing (%.2f%% vs Target %.2f%%)"), Result.DropsPerKillPct, ExpectedRatePct);
	}
	else if (Result.DropsPerKillPct < LowerBound)
	{
		Result.SummaryStatus = FString::Printf(TEXT("⚠️ Low Drop Rate (%.2f%% vs Target %.2f%%)"), Result.DropsPerKillPct, ExpectedRatePct);
	}
	else
	{
		Result.SummaryStatus = FString::Printf(TEXT("⚠️ High Drop Rate (%.2f%% vs Target %.2f%%)"), Result.DropsPerKillPct, ExpectedRatePct);
	}

	return Result;
}
