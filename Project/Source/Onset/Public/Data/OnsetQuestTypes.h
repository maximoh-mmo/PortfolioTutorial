// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Data/OnsetItemTypes.h"
#include "OnsetQuestTypes.generated.h"

/** What an objective tracks. Open enum: new objective kinds are a new value +
 *  one signal call site (ReportObjectiveProgress), never a new struct. */
UENUM(BlueprintType)
enum class EOnsetQuestObjectiveType : uint8
{
	Kill			UMETA(DisplayName = "Kill"),			// Target = DT_EnemyStats row name
	Collect			UMETA(DisplayName = "Collect"),		// Target = item row name (any category table)
	Interact		UMETA(DisplayName = "Interact"),	// Target = NPC / interactable ID (wired when an NPC system lands)
	ReachLocation	UMETA(DisplayName = "Reach Location"), // Target optional; uses Location + Radius
	UseItem			UMETA(DisplayName = "Use Item"),	// Target = item row name; count = uses
	Generic			UMETA(DisplayName = "Generic")		// Fired manually (BP/console); Target = free-form ID
};

/** One quest reward item grant (category table + row + count). */
USTRUCT(BlueprintType)
struct FOnsetQuestRewardItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	EOnsetItemCategory Category = EOnsetItemCategory::Equipment;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FName RowName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest", meta = (ClampMin = "1"))
	int32 Count = 1;
};

/** One stage objective. Completion is always the derived boolean Progress >= Count. */
USTRUCT(BlueprintType)
struct FOnsetQuestObjectiveDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	EOnsetQuestObjectiveType Type = EOnsetQuestObjectiveType::Generic;

	/** Type-interpreted ID: enemy row (Kill), item row (Collect/UseItem), npc ID (Interact),
	 *  or free-form (Generic). Empty = matches any target of the given type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FName Target;

	/** Required completions; clamped to >= 1 at load. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest", meta = (ClampMin = "1"))
	int32 Count = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FText Description;

	/** ReachLocation only: where the player must arrive. Ignored for other types. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FVector Location = FVector::ZeroVector;

	/** ReachLocation only: acceptance radius around Location. Ignored for other types. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest", meta = (ClampMin = "0"))
	float Radius = 200.0f;
};

/** One quest stage: 1..N objectives. Completes when every objective completes. */
USTRUCT(BlueprintType)
struct FOnsetQuestStageDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	TArray<FOnsetQuestObjectiveDefinition> Objectives;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FText Description;
};

/** One row in DT_Quests. RowName is the stable quest ID (e.g. "FirstBlood"). */
USTRUCT(BlueprintType)
struct FOnsetQuestDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FText Description;

	/** Stages run in order; the quest auto-advances when a stage's objectives all complete. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	TArray<FOnsetQuestStageDefinition> Stages;

	/** XP granted on quest completion via the existing progression pipeline. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest", meta = (ClampMin = "0"))
	int32 XpReward = 0;

	/** Items granted on quest completion via the player pawn's inventory. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	TArray<FOnsetQuestRewardItem> RewardItems;
};

// --- Runtime state (replicated on the quest component) ---

/** One tracked objective's live progress. */
USTRUCT(BlueprintType)
struct FOnsetQuestObjectiveState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	int32 Progress = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	bool bComplete = false;
};

/** One tracked stage's live progress (objectives parallel FOnsetQuestStageDefinition). */
USTRUCT(BlueprintType)
struct FOnsetQuestStageState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	bool bComplete = false;

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	TArray<FOnsetQuestObjectiveState> Objectives;
};

/** Live state of one accepted quest on the component. */
USTRUCT(BlueprintType)
struct FOnsetQuestState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	FName QuestRowName;

	/** Index into the quest definition's Stages of the currently active stage. */
	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	int32 StageIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	bool bComplete = false;

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	TArray<FOnsetQuestStageState> Stages;
};