// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/OnsetItemTypes.h"
#include "Data/OnsetQuestTypes.h"
#include "UOnsetQuestComponent.generated.h"

/** Broadcast on any quest state change (authority mutation or OnRep). */
DECLARE_MULTICAST_DELEGATE(FOnsetQuestsChanged);

class UOnsetInventoryComponent;
class AOnsetBaseCharacter;

/**
 * Server-authoritative quest tracker on the player pawn.
 *
 * Holds a replicated list of accepted quests and matches a generic
 * ReportObjectiveProgress(Type, Target, Amount) signal against the active
 * stage's objectives. Stages auto-advance when every objective completes;
 * finishing the last stage grants the quest's XP + item rewards.
 *
 * The row name in DT_Quests is the stable quest ID. Objective matching is
 * type + optional target, so quests stay data-driven: "kill 10 orcs",
 * "collect 3 stones", and "reach the shrine" are all the same pipeline.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ONSET_API UOnsetQuestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOnsetQuestComponent();

	/** Accepts a quest (server-only). No-op if already accepted or row missing. */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void AcceptQuest(FName QuestRowName);

	/** Force-completes a quest, granting rewards (server-only, dev/console). */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void CompleteQuest(FName QuestRowName);

	UFUNCTION(BlueprintPure, Category = "Quest")
	bool HasQuest(FName QuestRowName) const;

	/** Live quest states (parallel DT_Quests rows). */
	UFUNCTION(BlueprintPure, Category = "Quest")
	TArray<FOnsetQuestState> GetActiveQuests() const { return Quests; }

	/** C++-friendly const access to the replicated quest list. */
	const TArray<FOnsetQuestState>& GetQuestsRef() const { return Quests; }

	/**
	 * Generic progress signal. Matches active objectives of Type whose Target
	 * matches (empty target = matches any). Server-only. Collect signals come
	 * from the inventory's OnItemAdded; Kill signals come from AOnsetEnemy::OnDeath.
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void ReportObjectiveProgress(EOnsetQuestObjectiveType Type, FName Target, int32 Amount = 1);

	/** Checkpoint-style signal for ReachLocation objectives (server-only). */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void ReportReachLocation(FVector InLocation);

	// --- Persistence ---

	UFUNCTION(BlueprintCallable, Category = "Quest")
	FString SerializeQuestsJSON() const;

	UFUNCTION(BlueprintCallable, Category = "Quest")
	void DeserializeQuestsJSON(const FString& JSON);

	/** Fired on any quest state change (authority mutation or OnRep). */
	FOnsetQuestsChanged OnQuestsChanged;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Adds progress to matching objectives; returns true if any objective changed. */
	bool ApplyProgress(EOnsetQuestObjectiveType Type, FName Target, int32 Amount);

	/** Re-derives stage/objective completion and advances stages where possible. */
	void RecomputeCompletion(bool bGrantRewards = true);

	/** Grants the quest's XP + item rewards (server-only). */
	void GrantRewards(const FOnsetQuestDefinition& Def);

	UFUNCTION()
	void OnRep_Quests();

	/** Collect signal: routes inventory additions into quest progress. */
	void HandleItemAdded(EOnsetItemCategory Category, FName RowName, int32 Count);

	/** Live accepted quests (owner-only replication). */
	UPROPERTY(ReplicatedUsing = OnRep_Quests)
	TArray<FOnsetQuestState> Quests;
};