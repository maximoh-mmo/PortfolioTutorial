// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/OnsetEquipmentLibrary.h"

#include "Data/OnsetClassInfoTypes.h"
#include "Engine/DataTable.h"
#include "Misc/ConfigCacheIni.h"
#include "UObject/StrongObjectPtr.h"

namespace OnsetEquipmentLibraryInternal
{
	TStrongObjectPtr<UDataTable> CachedEquipmentTable = nullptr;
	TStrongObjectPtr<UDataTable> CachedClassTable = nullptr;
	TStrongObjectPtr<UDataTable> CachedEnemyStatsTable = nullptr;
	TMap<FName, FOnsetEnemyStats> FallbackEnemyStats;
	bool bEnemyFallbacksInitialized = false;

	void EnsureEnemyFallbacks()
	{
		if (bEnemyFallbacksInitialized)
		{
			return;
		}
		bEnemyFallbacksInitialized = true;

		{
			FOnsetEnemyStats Grunt;
			Grunt.DisplayName = FText::FromString(TEXT("Academy Combat Drone"));
			Grunt.Level = 1;
			Grunt.MaxHealth = 80.0f;
			Grunt.DamageBase = 20.0f;
			Grunt.Defense = 5.0f;
			Grunt.XpReward = 3;
			Grunt.ElementAffinity = EOnsetDamageElement::Physical;
			FallbackEnemyStats.Add(FName(TEXT("Grunt")), Grunt);
		}

		{
			FOnsetEnemyStats BioStalker;
			BioStalker.DisplayName = FText::FromString(TEXT("Bio-Stalker"));
			BioStalker.Level = 1;
			BioStalker.MaxHealth = 80.0f;
			BioStalker.DamageBase = 18.0f;
			BioStalker.Defense = 5.0f;
			BioStalker.XpReward = 4;
			BioStalker.ElementAffinity = EOnsetDamageElement::Physical;
			FallbackEnemyStats.Add(FName(TEXT("Academy_Lv1_BioStalker")), BioStalker);
		}

		{
			FOnsetEnemyStats RogueDrone;
			RogueDrone.DisplayName = FText::FromString(TEXT("Corrosive Maintenance Drone"));
			RogueDrone.Level = 2;
			RogueDrone.MaxHealth = 130.0f;
			RogueDrone.DamageBase = 24.0f;
			RogueDrone.Defense = 10.0f;
			RogueDrone.ResistancePoison = 25.0f;
			RogueDrone.XpReward = 5;
			RogueDrone.ElementAffinity = EOnsetDamageElement::Poison;
			FallbackEnemyStats.Add(FName(TEXT("Academy_Lv2_RogueDrone")), RogueDrone);
		}

		{
			FOnsetEnemyStats OutcastRaider;
			OutcastRaider.DisplayName = FText::FromString(TEXT("Outcast Salvage Raider"));
			OutcastRaider.Level = 3;
			OutcastRaider.MaxHealth = 180.0f;
			OutcastRaider.DamageBase = 32.0f;
			OutcastRaider.Defense = 15.0f;
			OutcastRaider.ResistanceIce = 25.0f;
			OutcastRaider.XpReward = 6;
			OutcastRaider.ElementAffinity = EOnsetDamageElement::Ice;
			FallbackEnemyStats.Add(FName(TEXT("Academy_Lv3_OutcastRaider")), OutcastRaider);
		}

		{
			FOnsetEnemyStats Excavator;
			Excavator.DisplayName = FText::FromString(TEXT("Heavy Excavator Unit"));
			Excavator.Level = 4;
			Excavator.MaxHealth = 260.0f;
			Excavator.DamageBase = 42.0f;
			Excavator.Defense = 22.0f;
			Excavator.ResistanceLightning = 30.0f;
			Excavator.XpReward = 8;
			Excavator.ElementAffinity = EOnsetDamageElement::Lightning;
			FallbackEnemyStats.Add(FName(TEXT("Academy_Lv4_ExcavatorUnit")), Excavator);
		}

		{
			FOnsetEnemyStats Aberration;
			Aberration.DisplayName = FText::FromString(TEXT("Rift Aberration"));
			Aberration.Level = 5;
			Aberration.MaxHealth = 380.0f;
			Aberration.DamageBase = 55.0f;
			Aberration.Defense = 30.0f;
			Aberration.ResistanceFire = 35.0f;
			Aberration.XpReward = 12;
			Aberration.ElementAffinity = EOnsetDamageElement::Fire;
			FallbackEnemyStats.Add(FName(TEXT("Academy_Lv5_RiftAberration")), Aberration);
		}
	}

	// Zone-tier K scale, read once from config and cached.
	float CachedZoneTierKScale = -1.0f;

	// Health per Vitality
	float CachedHealthPerVitality = -1.0f;

	// K values for mitigation
	float CachedKDefense = -1.0f;
	float CachedKElemental = -1.0f;

	// Block damage reduction
	float CachedBlockDamageReduction = -1.0f;
}

FString UOnsetEquipmentLibrary::GetEquipmentTablePath()
{
	FString Path;
	GConfig->GetString(TEXT("Onset.Gameplay"), TEXT("EquipmentDataTable"), Path, GEngineIni);
	if (Path.IsEmpty())
	{
		Path = TEXT("/Game/Game/Combat/DT_Equipment.DT_Equipment");
	}
	return Path;
}

UDataTable* UOnsetEquipmentLibrary::GetEquipmentTable()
{
	if (OnsetEquipmentLibraryInternal::CachedEquipmentTable)
	{
		return OnsetEquipmentLibraryInternal::CachedEquipmentTable.Get();
	}

	const FString TablePath = GetEquipmentTablePath();
	UDataTable* Table = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *TablePath));
	if (Table)
	{
		OnsetEquipmentLibraryInternal::CachedEquipmentTable = TStrongObjectPtr<UDataTable>(Table);
	}
	return Table;
}

const FOnsetEquipmentDefinition* UOnsetEquipmentLibrary::GetDefinition(FName RowName)
{
	UDataTable* Table = GetEquipmentTable();
	if (!Table || RowName.IsNone())
	{
		return nullptr;
	}
	return Table->FindRow<FOnsetEquipmentDefinition>(RowName, nullptr);
}

FString UOnsetEquipmentLibrary::GetClassDataTablePath()
{
	FString Path;
	GConfig->GetString(TEXT("Onset.Gameplay"), TEXT("ClassDataTable"), Path, GEngineIni);
	if (Path.IsEmpty())
	{
		Path = TEXT("/Game/Game/Combat/DT_ClassInfo.DT_ClassInfo");
	}
	return Path;
}

const FOnsetCharacterClassInfo* UOnsetEquipmentLibrary::GetClassInfo(EOnsetCharacterClass Class)
{
	if (!OnsetEquipmentLibraryInternal::CachedClassTable)
	{
		const FString TablePath = GetClassDataTablePath();
		UDataTable* Table = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *TablePath));
		if (Table)
		{
			OnsetEquipmentLibraryInternal::CachedClassTable = TStrongObjectPtr<UDataTable>(Table);
		}
	}

	UDataTable* ClassTable = OnsetEquipmentLibraryInternal::CachedClassTable.Get();
	if (!ClassTable)
	{
		return nullptr;
	}

	// Row names mirror the class display names (same convention as Server_CreateCharacter).
	const FName RowName = *UEnum::GetDisplayValueAsText(Class).ToString();
	return ClassTable->FindRow<FOnsetCharacterClassInfo>(RowName, nullptr);
}

FOnsetClassBaseStats UOnsetEquipmentLibrary::GetClassBaseStats(EOnsetCharacterClass Class)
{
	FOnsetClassBaseStats Base;
	if (const FOnsetCharacterClassInfo* Info = GetClassInfo(Class))
	{
		Base.Strength = Info->BaseStrength;
		Base.Intellect = Info->BaseIntellect;
		Base.Vitality = Info->BaseVitality;
		Base.Defense = Info->BaseDefense;
		Base.Agility = Info->BaseAgility;
		Base.Luck = Info->BaseLuck;
	}
	return Base;
}

FOnsetEquipmentDefinition UOnsetEquipmentLibrary::MakeDefaultWeaponForClass(EOnsetCharacterClass Class)
{
	FOnsetEquipmentDefinition Weapon;
	Weapon.Slot = EOnsetEquipmentSlot::Weapon;
	Weapon.DamageElement = EOnsetDamageElement::Physical;
	Weapon.WeaponDamage = GetDefaultWeaponDamage();

	switch (Class)
	{
		case EOnsetCharacterClass::Ranged:
			Weapon.Archetype = EOnsetWeaponArchetype::Bow;
			Weapon.DisplayName = NSLOCTEXT("OnsetEquipment", "DefaultBow", "Wooden Bow");
			break;

		case EOnsetCharacterClass::Support:
			// Casters have a weak weapon-scaled basic attack; damage comes from skills.
			Weapon.Archetype = EOnsetWeaponArchetype::Staff;
			Weapon.WeaponDamage = 18.0f;
			Weapon.DisplayName = NSLOCTEXT("OnsetEquipment", "DefaultStaff", "Apprentice Staff");
			break;

		case EOnsetCharacterClass::Tank:
		case EOnsetCharacterClass::DPS:
		default:
			Weapon.Archetype = EOnsetWeaponArchetype::Sword;
			Weapon.DisplayName = NSLOCTEXT("OnsetEquipment", "DefaultSword", "Iron Sword");
			break;
	}

	return Weapon;
}

float UOnsetEquipmentLibrary::GetDefaultWeaponDamage()
{
	return 25.0f;
}

float UOnsetEquipmentLibrary::GetArchetypeBaseCooldown(EOnsetWeaponArchetype Archetype)
{
	switch (Archetype)
	{
		case EOnsetWeaponArchetype::Dagger:		return 0.8f;
		case EOnsetWeaponArchetype::Wand:		return 0.9f;
		case EOnsetWeaponArchetype::Sword:		return 1.0f;
		case EOnsetWeaponArchetype::Bow:		return 1.0f;
		case EOnsetWeaponArchetype::Axe:		return 1.1f;
		case EOnsetWeaponArchetype::Mace:		return 1.1f;
		case EOnsetWeaponArchetype::Staff:		return 1.2f;
		case EOnsetWeaponArchetype::Tome:		return 1.3f;
		case EOnsetWeaponArchetype::Greatsword: return 1.8f;
	}
	return 1.0f;
}

bool UOnsetEquipmentLibrary::IsMeleeArchetype(EOnsetWeaponArchetype Archetype)
{
	switch (Archetype)
	{
		case EOnsetWeaponArchetype::Sword:
		case EOnsetWeaponArchetype::Dagger:
		case EOnsetWeaponArchetype::Axe:
		case EOnsetWeaponArchetype::Mace:
		case EOnsetWeaponArchetype::Greatsword:
			return true;
		default:
			return false;
	}
}

float UOnsetEquipmentLibrary::GetEnemyDifficultyMultiplier(int32 Tier)
{
	return FMath::Pow(1.0f + GetEnemyDifficultyGrowth(), static_cast<float>(FMath::Max(0, Tier)));
}

float UOnsetEquipmentLibrary::GetPrestigeMultiplier(int32 PrestigeLevel)
{
	return FMath::Pow(1.0f + GetPrestigeGrowth(), static_cast<float>(FMath::Max(0, PrestigeLevel)));
}

float UOnsetEquipmentLibrary::GetZoneTierKScale()
{
	if (OnsetEquipmentLibraryInternal::CachedZoneTierKScale < 0.0f)
	{
		float Scale = 1.0f;
		GConfig->GetFloat(TEXT("Onset.Gameplay"), TEXT("KZoneTierScale"), Scale, GEngineIni);
		OnsetEquipmentLibraryInternal::CachedZoneTierKScale = FMath::Max(0.1f, Scale);
	}
	return OnsetEquipmentLibraryInternal::CachedZoneTierKScale;
}

FString UOnsetEquipmentLibrary::GetEnemyStatsTablePath()
{
	FString Path;
	GConfig->GetString(TEXT("Onset.Gameplay"), TEXT("EnemyStatsDataTable"), Path, GEngineIni);
	if (Path.IsEmpty())
	{
		Path = TEXT("/Game/Game/Combat/DT_EnemyStats.DT_EnemyStats");
	}
	return Path;
}

UDataTable* UOnsetEquipmentLibrary::GetEnemyStatsTable()
{
	if (OnsetEquipmentLibraryInternal::CachedEnemyStatsTable)
	{
		return OnsetEquipmentLibraryInternal::CachedEnemyStatsTable.Get();
	}

	const FString TablePath = GetEnemyStatsTablePath();
	UDataTable* Table = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *TablePath));
	if (Table)
	{
		OnsetEquipmentLibraryInternal::CachedEnemyStatsTable = TStrongObjectPtr<UDataTable>(Table);
	}
	return Table;
}

const FOnsetEnemyStats* UOnsetEquipmentLibrary::GetEnemyStats(FName RowName)
{
	if (RowName.IsNone())
	{
		return nullptr;
	}

	OnsetEquipmentLibraryInternal::EnsureEnemyFallbacks();

	if (UDataTable* Table = GetEnemyStatsTable())
	{
		if (const FOnsetEnemyStats* Row = Table->FindRow<FOnsetEnemyStats>(RowName, nullptr))
		{
			return Row;
		}
	}

	return OnsetEquipmentLibraryInternal::FallbackEnemyStats.Find(RowName);
}

float UOnsetEquipmentLibrary::GetHealthPerVitality()
{
	if (OnsetEquipmentLibraryInternal::CachedHealthPerVitality < 0.0f)
	{
		float Value = 10.0f;
		GConfig->GetFloat(TEXT("Onset.Gameplay"), TEXT("HealthPerVitality"), Value, GEngineIni);
		OnsetEquipmentLibraryInternal::CachedHealthPerVitality = FMath::Max(0.1f, Value);
	}
	return OnsetEquipmentLibraryInternal::CachedHealthPerVitality;
}

float UOnsetEquipmentLibrary::GetKDefense()
{
	if (OnsetEquipmentLibraryInternal::CachedKDefense < 0.0f)
	{
		float Value = 100.0f;
		GConfig->GetFloat(TEXT("Onset.Gameplay"), TEXT("KDefense"), Value, GEngineIni);
		OnsetEquipmentLibraryInternal::CachedKDefense = FMath::Max(0.1f, Value);
	}
	return OnsetEquipmentLibraryInternal::CachedKDefense;
}

float UOnsetEquipmentLibrary::GetKElemental()
{
	if (OnsetEquipmentLibraryInternal::CachedKElemental < 0.0f)
	{
		float Value = 80.0f;
		GConfig->GetFloat(TEXT("Onset.Gameplay"), TEXT("KElemental"), Value, GEngineIni);
		OnsetEquipmentLibraryInternal::CachedKElemental = FMath::Max(0.1f, Value);
	}
	return OnsetEquipmentLibraryInternal::CachedKElemental;
}

float UOnsetEquipmentLibrary::GetBlockDamageReduction()
{
	if (OnsetEquipmentLibraryInternal::CachedBlockDamageReduction < 0.0f)
	{
		float Value = 0.50f;
		GConfig->GetFloat(TEXT("Onset.Gameplay"), TEXT("BlockDamageReduction"), Value, GEngineIni);
		OnsetEquipmentLibraryInternal::CachedBlockDamageReduction = FMath::Clamp(Value, 0.0f, 1.0f);
	}
	return OnsetEquipmentLibraryInternal::CachedBlockDamageReduction;
}

// --- Quality Grades, Durability, Salvage & Infusion ---

float UOnsetEquipmentLibrary::GetGradeMultiplier(EOnsetQualityGrade Grade)
{
	switch (Grade)
	{
		case EOnsetQualityGrade::GradeD: return 0.85f;
		case EOnsetQualityGrade::GradeC: return 1.00f;
		case EOnsetQualityGrade::GradeB: return 1.15f;
		case EOnsetQualityGrade::GradeA: return 1.30f;
		case EOnsetQualityGrade::GradeS: return 1.50f;
	}
	return 1.0f;
}

EOnsetQualityGrade UOnsetEquipmentLibrary::GetNextGrade(EOnsetQualityGrade Grade)
{
	switch (Grade)
	{
		case EOnsetQualityGrade::GradeD: return EOnsetQualityGrade::GradeC;
		case EOnsetQualityGrade::GradeC: return EOnsetQualityGrade::GradeB;
		case EOnsetQualityGrade::GradeB: return EOnsetQualityGrade::GradeA;
		case EOnsetQualityGrade::GradeA: return EOnsetQualityGrade::GradeS;
		case EOnsetQualityGrade::GradeS: return EOnsetQualityGrade::GradeS;
	}
	return EOnsetQualityGrade::GradeS;
}

FText UOnsetEquipmentLibrary::GetGradeDisplayText(EOnsetQualityGrade Grade)
{
	switch (Grade)
	{
		case EOnsetQualityGrade::GradeD: return NSLOCTEXT("Onset", "GradeD", "Grade D (Surplus)");
		case EOnsetQualityGrade::GradeC: return NSLOCTEXT("Onset", "GradeC", "Grade C (Standard)");
		case EOnsetQualityGrade::GradeB: return NSLOCTEXT("Onset", "GradeB", "Grade B (Refined)");
		case EOnsetQualityGrade::GradeA: return NSLOCTEXT("Onset", "GradeA", "Grade A (Overclocked)");
		case EOnsetQualityGrade::GradeS: return NSLOCTEXT("Onset", "GradeS", "Grade S (Apex)");
	}
	return FText::GetEmpty();
}

FLinearColor UOnsetEquipmentLibrary::GetGradeColor(EOnsetQualityGrade Grade)
{
	switch (Grade)
	{
		case EOnsetQualityGrade::GradeD: return FLinearColor(0.55f, 0.55f, 0.55f);
		case EOnsetQualityGrade::GradeC: return FLinearColor(0.90f, 0.90f, 0.90f);
		case EOnsetQualityGrade::GradeB: return FLinearColor(0.12f, 0.80f, 0.45f);
		case EOnsetQualityGrade::GradeA: return FLinearColor(0.65f, 0.25f, 0.95f);
		case EOnsetQualityGrade::GradeS: return FLinearColor(1.00f, 0.82f, 0.15f);
	}
	return FLinearColor::White;
}

EOnsetQualityGrade UOnsetEquipmentLibrary::RollRandomGrade(float LuckBonus)
{
	const float NormalizedLuck = FMath::Clamp(LuckBonus * 0.05f, 0.0f, 15.0f);
	const float WeightD = FMath::Max(5.0f, 25.0f - NormalizedLuck * 1.5f);
	const float WeightC = 50.0f;
	const float WeightB = 20.0f + NormalizedLuck * 0.8f;
	const float WeightA = 4.5f + NormalizedLuck * 0.5f;
	const float WeightS = 0.5f + NormalizedLuck * 0.2f;

	const float Total = WeightD + WeightC + WeightB + WeightA + WeightS;
	const float Roll = FMath::FRandRange(0.0f, Total);

	float Acc = WeightD;
	if (Roll <= Acc) return EOnsetQualityGrade::GradeD;
	Acc += WeightC;
	if (Roll <= Acc) return EOnsetQualityGrade::GradeC;
	Acc += WeightB;
	if (Roll <= Acc) return EOnsetQualityGrade::GradeB;
	Acc += WeightA;
	if (Roll <= Acc) return EOnsetQualityGrade::GradeA;
	return EOnsetQualityGrade::GradeS;
}

FOnsetEquipmentInstanceData UOnsetEquipmentLibrary::RollNewEquipmentInstance(EOnsetItemRarity Rarity, float LuckBonus)
{
	FOnsetEquipmentInstanceData Instance;
	Instance.CurrentDurability = 100.0f;
	Instance.MaxDurability = 100.0f;
	Instance.InfusionProgress = 0.0f;
	Instance.InfusionLevel = 0;

	float RarityLuck = LuckBonus;
	switch (Rarity)
	{
		case EOnsetItemRarity::Uncommon: RarityLuck += 5.0f; break;
		case EOnsetItemRarity::Rare:     RarityLuck += 15.0f; break;
		case EOnsetItemRarity::Epic:     RarityLuck += 30.0f; break;
		case EOnsetItemRarity::Legendary:RarityLuck += 50.0f; break;
		default: break;
	}

	Instance.PrimaryStatGrade = RollRandomGrade(RarityLuck);
	Instance.SecondaryStatGrade = RollRandomGrade(RarityLuck);

	return Instance;
}

float UOnsetEquipmentLibrary::CalculateEffectiveWeaponDamage(const FOnsetEquipmentDefinition* Def, const FOnsetEquipmentInstanceData& Instance)
{
	if (!Def) return 0.0f;
	const float BaseDamage = Def->WeaponDamage;
	const float GradeMult = GetGradeMultiplier(Instance.PrimaryStatGrade);
	const float MalfunctionPenalty = Instance.IsMalfunctioning() ? 0.5f : 1.0f;
	return BaseDamage * GradeMult * MalfunctionPenalty;
}

float UOnsetEquipmentLibrary::CalculateEffectiveDefense(const FOnsetEquipmentDefinition* Def, const FOnsetEquipmentInstanceData& Instance)
{
	if (!Def) return 0.0f;
	const float BaseDef = Def->DefenseBonus;
	const float GradeMult = GetGradeMultiplier(Instance.PrimaryStatGrade);
	const float MalfunctionPenalty = Instance.IsMalfunctioning() ? 0.5f : 1.0f;
	return BaseDef * GradeMult * MalfunctionPenalty;
}

float UOnsetEquipmentLibrary::CalculateEffectiveStatBonus(float BaseBonus, const FOnsetEquipmentInstanceData& Instance)
{
	const float GradeMult = GetGradeMultiplier(Instance.SecondaryStatGrade);
	const float MalfunctionPenalty = Instance.IsMalfunctioning() ? 0.5f : 1.0f;
	return BaseBonus * GradeMult * MalfunctionPenalty;
}

void UOnsetEquipmentLibrary::CalculateRepairCost(const FOnsetEquipmentDefinition* Def, const FOnsetEquipmentInstanceData& Instance, int32& OutCredits, int32& OutScraps)
{
	const float MissingPercent = 1.0f - Instance.GetDurabilityPercent();
	if (MissingPercent <= KINDA_SMALL_NUMBER)
	{
		OutCredits = 0;
		OutScraps = 0;
		return;
	}

	const int32 BaseVal = Def && Def->BuyValue > 0 ? Def->BuyValue : 100;
	float RarityMult = 1.0f;
	if (Def)
	{
		switch (Def->Rarity)
		{
			case EOnsetItemRarity::Uncommon: RarityMult = 1.25f; break;
			case EOnsetItemRarity::Rare:     RarityMult = 1.60f; break;
			case EOnsetItemRarity::Epic:     RarityMult = 2.00f; break;
			case EOnsetItemRarity::Legendary:RarityMult = 2.50f; break;
			default: break;
		}
	}

	OutCredits = FMath::Max(5, FMath::CeilToInt(MissingPercent * (BaseVal * 0.25f) * RarityMult));
	OutScraps = FMath::Max(1, FMath::CeilToInt(MissingPercent * 5.0f * RarityMult));
}

void UOnsetEquipmentLibrary::GetSalvageYield(const FOnsetEquipmentDefinition* Def, const FOnsetEquipmentInstanceData& Instance, TArray<FOnsetInventoryEntry>& OutYield)
{
	OutYield.Reset();
	const EOnsetItemRarity Rarity = Def ? Def->Rarity : EOnsetItemRarity::Common;

	switch (Rarity)
	{
		case EOnsetItemRarity::Common:
		{
			FOnsetInventoryEntry Scrap;
			Scrap.Category = EOnsetItemCategory::Material;
			Scrap.RowName = FName("CompositeScrap");
			Scrap.Count = FMath::RandRange(2, 4);
			OutYield.Add(Scrap);
			break;
		}
		case EOnsetItemRarity::Uncommon:
		{
			FOnsetInventoryEntry Scrap;
			Scrap.Category = EOnsetItemCategory::Material;
			Scrap.RowName = FName("CompositeScrap");
			Scrap.Count = FMath::RandRange(3, 5);
			OutYield.Add(Scrap);

			FOnsetInventoryEntry Alloy;
			Alloy.Category = EOnsetItemCategory::Material;
			Alloy.RowName = FName("ReinforcedAlloy");
			Alloy.Count = FMath::RandRange(1, 2);
			OutYield.Add(Alloy);
			break;
		}
		case EOnsetItemRarity::Rare:
		{
			FOnsetInventoryEntry Alloy;
			Alloy.Category = EOnsetItemCategory::Material;
			Alloy.RowName = FName("ReinforcedAlloy");
			Alloy.Count = FMath::RandRange(3, 5);
			OutYield.Add(Alloy);

			FOnsetInventoryEntry Filament;
			Filament.Category = EOnsetItemCategory::Material;
			Filament.RowName = FName("NaniteFilament");
			Filament.Count = FMath::RandRange(1, 2);
			OutYield.Add(Filament);
			break;
		}
		case EOnsetItemRarity::Epic:
		{
			FOnsetInventoryEntry Filament;
			Filament.Category = EOnsetItemCategory::Material;
			Filament.RowName = FName("NaniteFilament");
			Filament.Count = FMath::RandRange(3, 5);
			OutYield.Add(Filament);

			FOnsetInventoryEntry Core;
			Core.Category = EOnsetItemCategory::Material;
			Core.RowName = FName("QuantumCore");
			Core.Count = FMath::RandRange(1, 2);
			OutYield.Add(Core);
			break;
		}
		case EOnsetItemRarity::Legendary:
		{
			FOnsetInventoryEntry Filament;
			Filament.Category = EOnsetItemCategory::Material;
			Filament.RowName = FName("NaniteFilament");
			Filament.Count = FMath::RandRange(5, 8);
			OutYield.Add(Filament);

			FOnsetInventoryEntry Core;
			Core.Category = EOnsetItemCategory::Material;
			Core.RowName = FName("QuantumCore");
			Core.Count = FMath::RandRange(2, 4);
			OutYield.Add(Core);
			break;
		}
	}

	if (Def)
	{
		if (Def->Slot == EOnsetEquipmentSlot::Weapon)
		{
			FOnsetInventoryEntry WeaponParts;
			WeaponParts.Category = EOnsetItemCategory::Material;
			WeaponParts.RowName = FName("WeaponChassis");
			WeaponParts.Count = FMath::RandRange(1, 2);
			OutYield.Add(WeaponParts);
		}
		else if (Def->Slot == EOnsetEquipmentSlot::Shield ||
				 Def->Slot == EOnsetEquipmentSlot::Head ||
				 Def->Slot == EOnsetEquipmentSlot::Chest ||
				 Def->Slot == EOnsetEquipmentSlot::Hands ||
				 Def->Slot == EOnsetEquipmentSlot::Legs ||
				 Def->Slot == EOnsetEquipmentSlot::Feet)
		{
			FOnsetInventoryEntry ArmorParts;
			ArmorParts.Category = EOnsetItemCategory::Material;
			ArmorParts.RowName = FName("ArmorPlating");
			ArmorParts.Count = FMath::RandRange(1, 2);
			OutYield.Add(ArmorParts);
		}
	}

	if (Instance.PrimaryStatGrade >= EOnsetQualityGrade::GradeA || Instance.SecondaryStatGrade >= EOnsetQualityGrade::GradeA)
	{
		FOnsetInventoryEntry Bonus;
		Bonus.Category = EOnsetItemCategory::Material;
		Bonus.RowName = FName("ReinforcedAlloy");
		Bonus.Count = (Instance.PrimaryStatGrade == EOnsetQualityGrade::GradeS) ? 2 : 1;
		OutYield.Add(Bonus);
	}
}

float UOnsetEquipmentLibrary::GetInfusionProgressPerDuplicate(EOnsetQualityGrade CurrentGrade)
{
	switch (CurrentGrade)
	{
		case EOnsetQualityGrade::GradeD: return 1.0f;
		case EOnsetQualityGrade::GradeC: return 0.5f;
		case EOnsetQualityGrade::GradeB: return 0.334f;
		case EOnsetQualityGrade::GradeA: return 0.20f;
		case EOnsetQualityGrade::GradeS: return 0.0f;
	}
	return 0.0f;
}

bool UOnsetEquipmentLibrary::RollCriticalResonance()
{
	return FMath::FRand() < 0.15f;
}
