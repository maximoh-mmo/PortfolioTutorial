#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "MHInteractionTypes.generated.h"

/**
 * Defines how an interactable can be triggered by an instigator.
 */
UENUM(BlueprintType)
enum class EMHInteractionTriggerMode : uint8
{
	/** Triggered only by explicit user intent (click/tap or interact button). */
	ExplicitOnly		UMETA(DisplayName = "Explicit Only"),

	/** Triggered automatically when a valid instigator overlaps the trigger volume. */
	OverlapOnly			UMETA(DisplayName = "Overlap Only"),

	/** Supports both explicit interaction and automatic overlap triggering. */
	ExplicitAndOverlap	UMETA(DisplayName = "Explicit And Overlap")
};

/**
 * Static/runtime metadata describing an interactable target.
 */
USTRUCT(BlueprintType)
struct MHINTERACTION_API FMHInteractionDefinition
{
	GENERATED_BODY()

	/** Stable identifier for quests, analytics, or script matching (e.g. "NPC_ElderVane", "Door_CryptGate"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MH|Interaction")
	FName InteractableId = NAME_None;

	/** Human-readable name of the target (e.g. "Elder Vane", "Ancient Lever"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MH|Interaction")
	FText DisplayName;

	/** Verb or prompt shown to the player (e.g. "Talk", "Open", "Loot", "Activate"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MH|Interaction")
	FText ActionPrompt = NSLOCTEXT("MHInteraction", "DefaultPrompt", "Interact");

	/** Maximum 3D/2D distance (in cm) from which the instigator can execute the interaction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MH|Interaction", meta = (ClampMin = "25.0"))
	float InteractionRange = 250.0f;

	/** How this interactable can be activated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MH|Interaction")
	EMHInteractionTriggerMode TriggerMode = EMHInteractionTriggerMode::ExplicitOnly;

	/** If true, server validation traces ECC_Visibility between instigator and target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MH|Interaction")
	bool bRequiresLineOfSight = false;
};

/**
 * Context passed into CanInteract and ExecuteInteraction calls.
 */
USTRUCT(BlueprintType)
struct MHINTERACTION_API FMHInteractionContext
{
	GENERATED_BODY()

	/** The pawn or actor initiating the interaction. */
	UPROPERTY(BlueprintReadWrite, Category = "MH|Interaction")
	TObjectPtr<AActor> InstigatorActor = nullptr;

	/** The controller owning the instigator actor (if any). */
	UPROPERTY(BlueprintReadWrite, Category = "MH|Interaction")
	TObjectPtr<AController> InstigatorController = nullptr;

	/** The actor being interacted with. */
	UPROPERTY(BlueprintReadWrite, Category = "MH|Interaction")
	TObjectPtr<AActor> TargetActor = nullptr;

	/** True if this interaction was initiated by an overlap volume rather than an explicit click/button press. */
	UPROPERTY(BlueprintReadWrite, Category = "MH|Interaction")
	bool bTriggeredByOverlap = false;
};
