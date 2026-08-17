// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/OnsetItemLibrary.h"

#include "Combat/OnsetEquipmentLibrary.h"
#include "Engine/World.h"
#include "Misc/ConfigCacheIni.h"

namespace OnsetItemLibraryInternal
{
	TStrongObjectPtr<UDataTable> CachedTables[static_cast<uint8>(EOnsetItemCategory::Scroll) + 1] = { nullptr, nullptr, nullptr, nullptr };
}

const TCHAR* OnsetItemLibraryInternal_CategoryConfigKey(EOnsetItemCategory Category)
{
	switch (Category)
	{
		case EOnsetItemCategory::Equipment:	return TEXT("EquipmentDataTable");
		case EOnsetItemCategory::QuestItem:	return TEXT("QuestItemsDataTable");
		case EOnsetItemCategory::Junk:		return TEXT("JunkDataTable");
		case EOnsetItemCategory::Scroll:	return TEXT("ScrollDataTable");
	}
	return TEXT("EquipmentDataTable");
}

const TCHAR* OnsetItemLibraryInternal_CategoryDefaultPath(EOnsetItemCategory Category)
{
	switch (Category)
	{
		case EOnsetItemCategory::Equipment:	return TEXT("/Game/Data/DT_Equipment.DT_Equipment");
		case EOnsetItemCategory::QuestItem:	return TEXT("/Game/Data/DT_QuestItems.DT_QuestItems");
		case EOnsetItemCategory::Junk:		return TEXT("/Game/Data/DT_Junk.DT_Junk");
		case EOnsetItemCategory::Scroll:	return TEXT("/Game/Data/DT_Scrolls.DT_Scrolls");
	}
	return TEXT("/Game/Data/DT_Equipment.DT_Equipment");
}

FString UOnsetItemLibrary::GetTablePath(EOnsetItemCategory Category)
{
	FString Path;
	GConfig->GetString(TEXT("Onset.Gameplay"), OnsetItemLibraryInternal_CategoryConfigKey(Category), Path, GEngineIni);
	if (Path.IsEmpty())
	{
		Path = OnsetItemLibraryInternal_CategoryDefaultPath(Category);
	}
	return Path;
}

UDataTable* UOnsetItemLibrary::GetTable(EOnsetItemCategory Category)
{
	const uint8 Index = static_cast<uint8>(Category);
	if (Index > static_cast<uint8>(EOnsetItemCategory::Scroll))
	{
		return nullptr;
	}

	if (OnsetItemLibraryInternal::CachedTables[Index])
	{
		return OnsetItemLibraryInternal::CachedTables[Index].Get();
	}

	const FString TablePath = GetTablePath(Category);
	UDataTable* Table = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *TablePath));
	if (Table)
	{
		OnsetItemLibraryInternal::CachedTables[Index] = TStrongObjectPtr<UDataTable>(Table);
	}
	return Table;
}

const FOnsetItemDefinition* UOnsetItemLibrary::GetItemDefinition(EOnsetItemCategory Category, FName RowName)
{
	UDataTable* Table = GetTable(Category);
	if (!Table || RowName.IsNone())
	{
		return nullptr;
	}

	switch (Category)
	{
		case EOnsetItemCategory::Equipment:
			return Table->FindRow<FOnsetEquipmentDefinition>(RowName, nullptr);
		case EOnsetItemCategory::QuestItem:
			return Table->FindRow<FOnsetQuestItemDefinition>(RowName, nullptr);
		case EOnsetItemCategory::Junk:
			return Table->FindRow<FOnsetJunkItemDefinition>(RowName, nullptr);
		case EOnsetItemCategory::Scroll:
			return Table->FindRow<FOnsetScrollDefinition>(RowName, nullptr);
	}
	return nullptr;
}

EOnsetItemCategory UOnsetItemLibrary::GetCategoryForTable(const UDataTable* Table)
{
	for (uint8 Index = 0; Index <= static_cast<uint8>(EOnsetItemCategory::Scroll); ++Index)
	{
		const EOnsetItemCategory Category = static_cast<EOnsetItemCategory>(Index);
		if (GetTable(Category) == Table)
		{
			return Category;
		}
	}
	return EOnsetItemCategory::Equipment;
}

int32 UOnsetItemLibrary::GetMaxStackSize(EOnsetItemCategory Category, FName RowName)
{
	const FOnsetItemDefinition* Definition = GetItemDefinition(Category, RowName);
	return Definition ? FMath::Max(1, Definition->MaxStackSize) : 1;
}

void UOnsetItemLibrary::AddStacked(TArray<FOnsetInventoryEntry>& Bag, EOnsetItemCategory Category, FName RowName, int32 Count)
{
	if (RowName.IsNone() || Count <= 0)
	{
		return;
	}

	const int32 MaxStack = GetMaxStackSize(Category, RowName);

	// Fill existing matching stacks first.
	for (FOnsetInventoryEntry& Entry : Bag)
	{
		if (Entry.Category != Category || Entry.RowName != RowName || Entry.Count >= MaxStack)
		{
			continue;
		}
		const int32 Room = MaxStack - Entry.Count;
		const int32 Added = FMath::Min(Room, Count);
		Entry.Count += Added;
		Count -= Added;
		if (Count <= 0)
		{
			return;
		}
	}

	// Remaining quantity opens fresh stacks.
	while (Count > 0)
	{
		FOnsetInventoryEntry NewEntry;
		NewEntry.Category = Category;
		NewEntry.RowName = RowName;
		NewEntry.Count = FMath::Min(MaxStack, Count);
		Bag.Add(NewEntry);
		Count -= NewEntry.Count;
	}
}