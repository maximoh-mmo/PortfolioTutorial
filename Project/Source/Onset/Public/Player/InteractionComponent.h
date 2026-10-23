// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MHInteractionTypes.h"
#include "InteractionComponent.generated.h"

struct FHitResult;

struct FMoveTarget
{
	TWeakObjectPtr<AActor> Actor = nullptr;
	FVector Position = FVector::ZeroVector;
	float AcceptanceRadius = 200.0f;
};

class AOnsetCorpse;
class AOnsetPlayerController;
class UTargetingComponent;

/**
 * Controller-owned component that bridges player input (click-to-move/interact and proximity
 * interact button) with the modular MHInteraction framework and combat targeting.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ONSET_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionComponent();

	/** Primary interaction: resolves interactables, combat targets, or ground navigation. */
	FMoveTarget ProcessPrimaryInteraction(AActor* HitActor, FVector HitLocation);

	FMoveTarget GetPendingMovementTarget() const { return PendingMovementTarget; }

	/** Attempts to interact with the currently focused proximity interactable (or closest valid one). */
	bool TryInteractWithFocused();

	/** Scans around the controlled pawn for nearby IMHInteractable actors and updates FocusedInteractable. */
	void UpdateProximityFocus(FMHInteractionDefinition& OutFocusedDefinition, bool& bOutHasFocus);

	/** Returns the currently focused in-range interactable actor (if any). */
	AActor* GetFocusedInteractable() const { return FocusedInteractableActor.Get(); }

	/** Default fallback range within which a corpse or interactable can be used on click. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float LootRange = 250.0f;

	/** Search radius around the player pawn for proximity interaction prompts. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float ProximitySearchRadius = 350.0f;

	/** Echo-poll rate while auto-pathing is waiting to arrive at an interactable. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractArrivalPollInterval = 0.15f;

	/** Clears any pending walk-up interaction and stops the arrival poll timer. */
	void ClearPendingInteraction();

	/** Legacy alias for ClearPendingInteraction. */
	void ClearPendingLoot() { ClearPendingInteraction(); }

private:
	/** Initiates immediate interaction if in range, or schedules walk-up arrival polling if out of range. */
	void TryInteractWithActor(AActor* TargetActor);
	void OnInteractArrivalTick();

	UPROPERTY()
	TObjectPtr<UTargetingComponent> TargetingComponent;

	FMoveTarget PendingMovementTarget = {};

	/** The interactable actor we are currently walking up to. */
	TWeakObjectPtr<AActor> PendingInteractActor;

	/** Closest valid in-range interactable discovered by the proximity scan. */
	TWeakObjectPtr<AActor> FocusedInteractableActor;

	FTimerHandle InteractArrivalTimerHandle;
};
