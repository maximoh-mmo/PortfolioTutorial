// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "OnsetDialogueTypes.generated.h"

/**
 * A single player response choice within a dialogue node.
 * Can optionally gate its visibility by quest state, grant/complete quests on selection,
 * report a generic objective event, and branch to another dialogue row (or close if None).
 */
USTRUCT(BlueprintType)
struct ONSET_API FOnsetDialogueChoice
{
	GENERATED_BODY()

	/** Text displayed on the choice button (e.g. "I will help you.", "Farewell."). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FText ResponseText;

	/** Next row in DT_Dialogues to transition to; NAME_None closes the dialogue window. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FName NextDialogueRow = NAME_None;

	/** Optional condition: only show this choice if the player has NOT yet accepted this quest. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Conditions")
	FName RequiredQuestNotStarted = NAME_None;

	/** Optional condition: only show this choice if the player currently has this quest active (accepted & incomplete). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Conditions")
	FName RequiredQuestActive = NAME_None;

	/** Optional condition: only show this choice if the player has completed this quest. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Conditions")
	FName RequiredQuestComplete = NAME_None;

	/** Server action: accepts this quest row in DT_Quests when the player picks this choice. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Actions")
	FName GrantQuestOnSelect = NAME_None;

	/** Server action: force-completes / turns in this quest row when the player picks this choice. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Actions")
	FName CompleteQuestOnSelect = NAME_None;

	/** Server action: reports EOnsetQuestObjectiveType::Generic progress for this target ID when selected. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Actions")
	FName ReportGenericObjectiveTarget = NAME_None;
};

/**
 * One row in DT_Dialogues representing a single dialogue node (speaker + body text + choices).
 */
USTRUCT(BlueprintType)
struct ONSET_API FOnsetDialogueRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Display name of the speaker shown in the dialogue header. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FText SpeakerName;

	/** Main body text spoken by the NPC. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue", meta = (MultiLine = "true"))
	FText DialogueText;

	/** Available player response choices. If empty, a default "Farewell" close option is shown. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	TArray<FOnsetDialogueChoice> Choices;
};

/**
 * Optional quest-state greeting override for an NPC so an NPC can greet the player differently
 * before a quest is accepted, while it is in progress, and after it is completed.
 */
USTRUCT(BlueprintType)
struct ONSET_API FOnsetNPCQuestDialogueOverride
{
	GENERATED_BODY()

	/** The quest row in DT_Quests that drives this greeting branch. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FName QuestRowName = NAME_None;

	/** Dialogue row used when the player has not yet accepted QuestRowName. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FName NotStartedDialogueRow = NAME_None;

	/** Dialogue row used while QuestRowName is active (accepted, not complete). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FName InProgressDialogueRow = NAME_None;

	/** Dialogue row used after QuestRowName is completed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FName CompletedDialogueRow = NAME_None;
};

/**
 * One row in DT_NPCs defining a non-enemy NPC's identity, prompt, and dialogue entry points.
 */
USTRUCT(BlueprintType)
struct ONSET_API FOnsetNPCDefinition : public FTableRowBase
{
	GENERATED_BODY()

	/** Display name shown on the interaction prompt and overhead nameplate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FText DisplayName;

	/** Optional subtitle/role (e.g. "Village Elder", "Blacksmith"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FText TitleOrRole;

	/** Interaction verb shown on prompt (defaults to "Talk"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FText InteractionPrompt = NSLOCTEXT("OnsetNPC", "DefaultTalkPrompt", "Talk");

	/** Default entry row in DT_Dialogues when no quest override matches. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FName DefaultDialogueRow = NAME_None;

	/** Evaluated in order: the first matching quest override supplies the starting dialogue row. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	TArray<FOnsetNPCQuestDialogueOverride> QuestDialogueOverrides;
};
