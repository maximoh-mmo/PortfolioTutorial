// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/OnsetQuestLibrary.h"

#include "Engine/DataTable.h"
#include "Misc/ConfigCacheIni.h"

namespace OnsetQuestLibraryInternal
{
	TStrongObjectPtr<UDataTable> CachedTable;
}

FString UOnsetQuestLibrary::GetTablePath()
{
	FString Path;
	GConfig->GetString(TEXT("Onset.Gameplay"), TEXT("QuestDataTable"), Path, GEngineIni);
	if (Path.IsEmpty())
	{
		Path = TEXT("/Game/Data/DT_Quests.DT_Quests");
	}
	return Path;
}

UDataTable* UOnsetQuestLibrary::GetTable()
{
	if (OnsetQuestLibraryInternal::CachedTable)
	{
		return OnsetQuestLibraryInternal::CachedTable.Get();
	}

	const FString TablePath = GetTablePath();
	UDataTable* Table = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *TablePath));
	if (Table)
	{
		OnsetQuestLibraryInternal::CachedTable = TStrongObjectPtr<UDataTable>(Table);
	}
	return Table;
}

const FOnsetQuestDefinition* UOnsetQuestLibrary::GetQuest(FName RowName)
{
	UDataTable* Table = GetTable();
	if (!Table || RowName.IsNone())
	{
		return nullptr;
	}

	return Table->FindRow<FOnsetQuestDefinition>(RowName, nullptr);
}