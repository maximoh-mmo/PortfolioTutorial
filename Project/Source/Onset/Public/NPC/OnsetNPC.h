// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/OnsetDialogueTypes.h"
#include "GameFramework/Character.h"
#include "MHInteractionTypes.h"
#include "OnsetNPC.generated.h"

class AOnsetPlayerController;
class UMHInteractableComponent;
class UOnsetQuestComponent;

/**
 * Non-enemy interactable NPC (Quest giver, dialogue speaker, vendor/guide).
 * Uses UMHInteractableComponent from the MHInteraction framework plugin for world interaction
 * and connects to UOnsetQuestComponent + UOnsetDialogueLibrary for quests and branching dialogue.
 */
UCLASS(Blueprintable)
class ONSET_API AOnsetNPC : public ACharacter
{
	GENERATED_BODY()

public:
	AOnsetNPC();

	/** Modular MH Framework interaction component. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Interaction")
	TObjectPtr<UMHInteractableComponent> InteractableComponent;

	/** Row name in DT_NPCs (or runtime fallback, e.g. "ElderVane"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "NPC")
	FName NPCRowName = FName(TEXT("ElderVane"));

	/** If true, InlineNPCDefinition is used instead of looking up NPCRowName in DT_NPCs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "NPC")
	bool bUseInlineDefinition = false;

	/** Per-instance NPC definition when bUseInlineDefinition is true (or when NPCRowName is not found). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "NPC", meta = (EditCondition = "bUseInlineDefinition"))
	FOnsetNPCDefinition InlineNPCDefinition;

	/** If true, the NPC rotates to face the player who interacts with them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	bool bFaceInteractingPlayer = true;

	/** Returns the active NPC definition (from DT_NPCs or InlineNPCDefinition). */
	UFUNCTION(BlueprintPure, Category = "NPC")
	FOnsetNPCDefinition GetEffectiveNPCDefinition() const;

	/** Resolves the starting dialogue row for the given player's quest state. */
	UFUNCTION(BlueprintPure, Category = "NPC")
	FName ResolveStartingDialogueRow(const UOnsetQuestComponent* QuestComponent) const;

	/** Server-side handler when a player selects ChoiceIndex on CurrentDialogueRow while speaking to this NPC. */
	void HandleDialogueChoiceSelected(AOnsetPlayerController* InteractingPC, FName CurrentDialogueRow, int32 ChoiceIndex);

	/** Blueprint hook fired on the server whenever a player interacts with this NPC. */
	UFUNCTION(BlueprintImplementableEvent, Category = "NPC")
	void BP_OnPlayerInteracted(AActor* InstigatorPawn);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	void HandleInteracted(const FMHInteractionContext& Context);
	void ModifyInteractionDefinition(FMHInteractionDefinition& InOutDefinition);
};
