// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/OnsetLootLibrary.h"

#include "Data/OnsetItemLibrary.h"
#include "Engine/World.h"
#include "Misc/ConfigCacheIni.h"

namespace OnsetLootLibraryInternal
{
	TStrongObjectPtr<UDataTable> CachedLootTable = nullptr;

	constexpr int32 MaxSubTableDepth = 8;
}

FString UOnsetLootLibrary::GetLootTablePath()
{
	FString Path;
	GConfig->GetString(TEXT("Onset.Gameplay"), TEXT("LootDataTable"), Path, GEngineIni);
	if (Path.IsEmpty())
	{
		Path = TEXT("/Game/Data/DT_Loot.DT_Loot");
	}
	return Path;
}

UDataTable* UOnsetLootLibrary::GetLootTable()
{
	if (OnsetLootLibraryInternal::CachedLootTable)
	{
		return OnsetLootLibraryInternal::CachedLootTable.Get();
	}

	const FString TablePath = GetLootTablePath();
	UDataTable* Table = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *TablePath));
	if (Table)
	{
		OnsetLootLibraryInternal::CachedLootTable = TStrongObjectPtr<UDataTable>(Table);
	}
	return Table;
}

namespace OnsetLootLibraryInternal
{
	void ExpandTable(const UDataTable* Table, const FName& RowName, const FOnsetLootContext& Context,
		TArray<const FOnsetLootEntry*>& OutEntries, TSet<FName>& Visited, int32 Depth)
	{
		if (!Table || RowName.IsNone() || Depth >= MaxSubTableDepth || Visited.Contains(RowName))
		{
			return;
		}
		Visited.Add(RowName);

		const FOnsetLootTableRow* Row = Table->FindRow<FOnsetLootTableRow>(RowName, nullptr);
		if (!Row)
		{
			return;
		}

		for (const FOnsetLootSubTableRef& Ref : Row->SubTables)
		{
			if (Ref.Table.DataTable && FMath::FRand() <= Ref.InclusionChance)
			{
				ExpandTable(Ref.Table.DataTable, Ref.Table.RowName, Context, OutEntries, Visited, Depth + 1);
			}
		}

		for (const FOnsetLootEntry& Entry : Row->Entries)
		{
			if (!Entry.Item.DataTable || Entry.Item.RowName.IsNone())
			{
				continue;
			}
			if (Context.Level > 0 &&
				((Entry.MinLevel > 0 && Context.Level < Entry.MinLevel) ||
				 (Entry.MaxLevel > 0 && Context.Level > Entry.MaxLevel)))
			{
				continue;
			}
			if (Entry.RequiredZoneTag.IsValid() && Entry.RequiredZoneTag != Context.ZoneTag)
			{
				continue;
			}
			OutEntries.Add(&Entry);
		}
	}
}

TArray<FOnsetInventoryEntry> UOnsetLootLibrary::RollLoot(const FDataTableRowHandle& Table, const FOnsetLootContext& Context)
{
	TArray<FOnsetInventoryEntry> Loot;

	if (!Table.DataTable || Table.RowName.IsNone())
	{
		return Loot;
	}

	TArray<const FOnsetLootEntry*> Entries;
	TSet<FName> Visited;
	OnsetLootLibraryInternal::ExpandTable(Table.DataTable, Table.RowName, Context, Entries, Visited, 0);

	for (const FOnsetLootEntry* Entry : Entries)
	{
		if (FMath::FRand() > Entry->DropChance)
		{
			continue;
		}
		const int32 Quantity = FMath::Max(1, FMath::RandRange(Entry->MinQty, FMath::Max(Entry->MinQty, Entry->MaxQty)));
		const EOnsetItemCategory Category = UOnsetItemLibrary::GetCategoryForTable(Entry->Item.DataTable);
		UOnsetItemLibrary::AddStacked(Loot, Category, Entry->Item.RowName, Quantity);
	}

	return Loot;
}

TArray<FOnsetInventoryEntry> UOnsetLootLibrary::RollLoot(FName TableRowName, const FOnsetLootContext& Context)
{
	FDataTableRowHandle Handle;
	Handle.DataTable = GetLootTable();
	Handle.RowName = TableRowName;
	return RollLoot(Handle, Context);
}