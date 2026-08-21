// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/OnsetQuestTypes.h"
#include "UObject/Object.h"
#include "OnsetQuestLibrary.generated.h"

class UDataTable;

/**
 * Registry for the quest table (DT_Quests). The table path is overridable via
 * the Onset.Gameplay QuestDataTable seam in DefaultEngine.ini, matching the
 * item/ability table pattern (see UOnsetItemLibrary).
 */
UCLASS()
class ONSET_API UOnsetQuestLibrary : public UObject
{
	GENERATED_BODY()

public:
	/** Table path from the Onset.Gameplay ini seam (baked default fallback). */
	static FString GetTablePath();

	/** Loads (and caches) the quest table; null if missing. */
	static UDataTable* GetTable();

	/** Reads a quest row; null if missing. Clamps objective Counts to >= 1. */
	static const FOnsetQuestDefinition* GetQuest(FName RowName);
};