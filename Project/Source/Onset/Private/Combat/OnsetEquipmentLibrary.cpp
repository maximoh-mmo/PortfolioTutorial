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