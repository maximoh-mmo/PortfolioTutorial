// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/OnsetAbilityLibrary.h"

#include "Combat/OnsetGameplayAbility.h"
#include "Data/OnsetAbilityTypes.h"
#include "Engine/DataTable.h"
#include "GameplayTagsManager.h"
#include "Misc/ConfigCacheIni.h"
#include "UObject/StrongObjectPtr.h"

namespace OnsetAbilityLibraryInternal
{
	TStrongObjectPtr<UDataTable> CachedTable = nullptr;
	TMap<FName, FOnsetAbilityDefinition> CachedRows;
}

FString UOnsetAbilityLibrary::GetAbilityDataTablePath()
{
	FString Path;
	GConfig->GetString(TEXT("Onset.Gameplay"), TEXT("AbilityDataTable"), Path, GEngineIni);
	if (Path.IsEmpty())
	{
		Path = TEXT("/Game/Game/Combat/DT_Abilities.DT_Abilities");
	}
	return Path;
}

UDataTable* UOnsetAbilityLibrary::GetAbilityTable()
{
	if (OnsetAbilityLibraryInternal::CachedTable)
	{
		return OnsetAbilityLibraryInternal::CachedTable.Get();
	}
	return LoadTable();
}

const FOnsetAbilityDefinition* UOnsetAbilityLibrary::GetDefinition(FName RowName)
{
	// Ensure the table + cache are populated.
	GetAbilityTable();
	return OnsetAbilityLibraryInternal::CachedRows.Find(RowName);
}

FGameplayTag UOnsetAbilityLibrary::MakeAbilityIDTag(FName RowName)
{
	return FGameplayTag::RequestGameplayTag(FName(*FString::Printf(TEXT("AbilityID.%s"), *RowName.ToString())));
}

const FOnsetAbilityDefinition* UOnsetAbilityLibrary::GetDefinitionFromDynamicTags(const FGameplayTagContainer& DynamicTags)
{
	// Ensure AbilityID.<RowName> tags are registered before scanning. On clients the
	// HUD can rebuild before the local character has run LoadTable(), so the tags may
	// be plain (unregistered) FNames; GetAbilityTable() also registers them.
	GetAbilityTable();

	for (const FGameplayTag& Tag : DynamicTags)
	{
		const FString TagString = Tag.ToString();
		FStringView TagView(TagString);
		FStringView PrefixView(TEXT("AbilityID."));
		if (TagView.StartsWith(PrefixView))
		{
			return GetDefinition(FName(TagView.RightChop(PrefixView.Len())));
		}
	}
	return nullptr;
}

TSubclassOf<UOnsetGameplayAbility> UOnsetAbilityLibrary::ResolveAbilityClass(FName RowName)
{
	const FOnsetAbilityDefinition* Definition = GetDefinition(RowName);
	if (!Definition)
	{
		UE_LOG(LogTemp, Error, TEXT("UOnsetAbilityLibrary: no definition for row '%s'"), *RowName.ToString());
		return nullptr;
	}

	if (Definition->AbilityClass.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("UOnsetAbilityLibrary: row '%s' has no AbilityClass assigned"), *RowName.ToString());
		return nullptr;
	}

	TSubclassOf<UOnsetGameplayAbility> Resolved = Definition->AbilityClass.LoadSynchronous();
	if (!Resolved)
	{
		UE_LOG(LogTemp, Error, TEXT("UOnsetAbilityLibrary: row '%s' AbilityClass failed to resolve (%s)"),
			*RowName.ToString(), *Definition->AbilityClass.ToString());
	}
	return Resolved;
}

bool UOnsetAbilityLibrary::ValidateDefinitions()
{
	UDataTable* Table = GetAbilityTable();
	if (!Table)
	{
		UE_LOG(LogTemp, Error, TEXT("UOnsetAbilityLibrary::ValidateDefinitions: unable to load ability table '%s'"), *GetAbilityDataTablePath());
		return false;
	}

	bool bAllValid = true;
	for (const TPair<FName, uint8*>& Entry : Table->GetRowMap())
	{
		const FName RowName = Entry.Key;
		const FOnsetAbilityDefinition* Definition = reinterpret_cast<const FOnsetAbilityDefinition*>(Entry.Value);
		if (!Definition)
		{
			UE_LOG(LogTemp, Error, TEXT("UOnsetAbilityLibrary: row '%s' has invalid row data"), *RowName.ToString());
			bAllValid = false;
			continue;
		}

		if (Definition->AbilityClass.IsNull())
		{
			UE_LOG(LogTemp, Error, TEXT("UOnsetAbilityLibrary: row '%s' has no AbilityClass"), *RowName.ToString());
			bAllValid = false;
		}

		for (const FOnsetAbilityEffect& Effect : Definition->Effects)
		{
			if (Effect.Type == EOnsetAbilityEffectType::Damage && !Effect.DamageTypeTag.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("UOnsetAbilityLibrary: row '%s' damage effect has no DamageTypeTag (defaults to physical)"), *RowName.ToString());
			}
			if (Effect.Type == EOnsetAbilityEffectType::Damage && Effect.Magnitude <= 0.0f)
			{
				UE_LOG(LogTemp, Warning, TEXT("UOnsetAbilityLibrary: row '%s' damage effect has non-positive magnitude"), *RowName.ToString());
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("UOnsetAbilityLibrary::ValidateDefinitions: %s (%d rows)"), bAllValid ? TEXT("OK") : TEXT("FAILED"), Table->GetRowMap().Num());
	return bAllValid;
}

void UOnsetAbilityLibrary::Refresh()
{
	OnsetAbilityLibraryInternal::CachedTable.Reset();
	OnsetAbilityLibraryInternal::CachedRows.Reset();
}

UDataTable* UOnsetAbilityLibrary::LoadTable()
{
	const FString TablePath = GetAbilityDataTablePath();
	UDataTable* Table = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *TablePath));
	if (!Table)
	{
		UE_LOG(LogTemp, Error, TEXT("UOnsetAbilityLibrary: failed to load ability DataTable '%s'"), *TablePath);
		return nullptr;
	}

	OnsetAbilityLibraryInternal::CachedTable = TStrongObjectPtr<UDataTable>(Table);
	OnsetAbilityLibraryInternal::CachedRows.Reset();
	OnsetAbilityLibraryInternal::CachedRows.Reserve(Table->GetRowMap().Num());
	for (const TPair<FName, uint8*>& Entry : Table->GetRowMap())
	{
		const FOnsetAbilityDefinition* Definition = reinterpret_cast<const FOnsetAbilityDefinition*>(Entry.Value);
		if (Definition)
		{
			OnsetAbilityLibraryInternal::CachedRows.Add(Entry.Key, *Definition);
		}

		// Register the AbilityID.<RowName> tag so MakeAbilityIDTag() finds it when
		// granting the spec. Rows are authored at runtime (editor tool), so the tag
		// can't be baked in as a native tag. AddNativeGameplayTag is idempotent and
		// safe to call after startup; it also creates the AbilityID parent chain.
		UGameplayTagsManager::Get().AddNativeGameplayTag(
			FName(*FString::Printf(TEXT("AbilityID.%s"), *Entry.Key.ToString())));
	}
	return Table;
}
