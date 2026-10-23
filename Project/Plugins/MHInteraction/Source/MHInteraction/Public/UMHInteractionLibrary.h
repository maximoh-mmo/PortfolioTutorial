#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MHInteractionTypes.h"
#include "UMHInteractionLibrary.generated.h"

/**
 * Stateless helper functions for resolving, validating, and executing interactions
 * against actors or components implementing IMHInteractable.
 */
UCLASS()
class MHINTERACTION_API UMHInteractionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Finds the UObject (Actor or ActorComponent) implementing UMHInteractable on TargetActor.
	 * Checks TargetActor directly first, then searches its components.
	 */
	UFUNCTION(BlueprintPure, Category = "MH|Interaction")
	static UObject* ResolveInteractableObject(AActor* TargetActor);

	/** Returns true if TargetActor (or one of its components) implements UMHInteractable. */
	UFUNCTION(BlueprintPure, Category = "MH|Interaction")
	static bool IsInteractableActor(AActor* TargetActor);

	/** Retrieves the FMHInteractionDefinition for TargetActor if it is interactable. */
	UFUNCTION(BlueprintPure, Category = "MH|Interaction")
	static bool GetInteractionDefinition(AActor* TargetActor, FMHInteractionDefinition& OutDefinition);

	/** Returns true if InstigatorActor is within TargetActor's configured InteractionRange (multiplied by RangeToleranceScale). */
	UFUNCTION(BlueprintPure, Category = "MH|Interaction")
	static bool IsWithinInteractionRange(const AActor* InstigatorActor, AActor* TargetActor, float RangeToleranceScale = 1.0f);

	/**
	 * Validates that Context has valid actors, passes IMHInteractable::CanInteract,
	 * is within range (with network tolerance), and satisfies line-of-sight if required.
	 */
	UFUNCTION(BlueprintPure, Category = "MH|Interaction")
	static bool ValidateInteraction(const FMHInteractionContext& Context, FText& OutFailureReason, float RangeToleranceScale = 1.15f);

	/** Validates and executes the interaction on TargetActor. */
	UFUNCTION(BlueprintCallable, Category = "MH|Interaction")
	static bool TryExecuteInteraction(const FMHInteractionContext& Context, FText& OutFailureReason);
};
