// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/OnsetDialogueTypes.h"
#include "UObject/Object.h"
#include "OnsetDialogueLibrary.generated.h"

class UDataTable;
class UOnsetQuestComponent;

/**
 * Registry for NPC definitions (DT_NPCs) and branching dialogue nodes (DT_Dialogues).
 * Supports DefaultEngine.ini path overrides under [Onset.Gameplay] (DialogueDataTable, NPCDataTable)
 * as well as built-in fallback rows so level placement works even before DataTable assets are authored.
 */
UCLASS()
class ONSET_API UOnsetDialogueLibrary : public UObject
{
	GENERATED_BODY()

public:
	static FString GetDialogueTablePath();
	static FString GetNPCTablePath();

	static UDataTable* GetDialogueTable();
	static UDataTable* GetNPCTable();

	/** Looks up a dialogue node by RowName in DT_Dialogues (falling back to registered runtime rows). */
	static const FOnsetDialogueRow* GetDialogueRow(FName RowName);

	/** Looks up an NPC definition by RowName in DT_NPCs (falling back to registered runtime rows). */
	static const FOnsetNPCDefinition* GetNPCDefinition(FName RowName);

	/** Registers or overrides a runtime fallback dialogue row (useful for code/level prototypes and tests). */
	static void RegisterFallbackDialogueRow(FName RowName, const FOnsetDialogueRow& Row);

	/** Registers or overrides a runtime fallback NPC definition. */
	static void RegisterFallbackNPCDefinition(FName RowName, const FOnsetNPCDefinition& Definition);

	/** Returns true if Choice's quest prerequisites are satisfied for the given QuestComponent. */
	static bool IsChoiceAvailable(const FOnsetDialogueChoice& Choice, const UOnsetQuestComponent* QuestComponent);

	/** Resolves the appropriate entry dialogue row for an NPC definition given the player's current quest state. */
	static FName ResolveEntryDialogueRow(const FOnsetNPCDefinition& NPCDef, const UOnsetQuestComponent* QuestComponent);
};
