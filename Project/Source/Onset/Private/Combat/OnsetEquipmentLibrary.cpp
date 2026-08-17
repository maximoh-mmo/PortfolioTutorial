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

	// Zone-tier K scale, read once from config and cached.
	float CachedZoneTierKScale = -1.0f;
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
	UDataTable* Table = GetEnemyStatsTable();
	if (!Table || RowName.IsNone())
	{
		return nullptr;
	}
	return Table->FindRow<FOnsetEnemyStats>(RowName, nullptr);
}