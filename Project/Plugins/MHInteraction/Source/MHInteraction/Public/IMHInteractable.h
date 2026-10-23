#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MHInteractionTypes.h"
#include "IMHInteractable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UMHInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Generic contract for any Actor or ActorComponent that can be interacted with.
 * Kept game-agnostic in MHInteraction; game modules consume this interface to implement
 * NPCs, doors, switches, chests, corpses, etc.
 */
class MHINTERACTION_API IMHInteractable
{
	GENERATED_BODY()

public:
	/** Populates OutDefinition with the interactable's display name, prompt, range, and trigger mode. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MH|Interaction")
	void GetInteractionDefinition(FMHInteractionDefinition& OutDefinition) const;
	virtual void GetInteractionDefinition_Implementation(FMHInteractionDefinition& OutDefinition) const
	{
	}

	/** Returns true if the instigator in Context is currently allowed to interact with this target. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MH|Interaction")
	bool CanInteract(const FMHInteractionContext& Context, FText& OutFailureReason) const;
	virtual bool CanInteract_Implementation(const FMHInteractionContext& Context, FText& OutFailureReason) const
	{
		return true;
	}

	/** Performs the interaction (called on server authority for networked actors). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MH|Interaction")
	bool ExecuteInteraction(const FMHInteractionContext& Context);
	virtual bool ExecuteInteraction_Implementation(const FMHInteractionContext& Context)
	{
		return false;
	}
};
