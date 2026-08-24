// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"


struct FHitResult;

struct FMoveTarget
{
	TWeakObjectPtr<AActor> Actor = nullptr;
	FVector Position = FVector::ZeroVector;
};

class AOnsetCorpse;
class AOnsetPlayerController;
class UTargetingComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ONSET_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionComponent();
	
	/** Primary interaction: raycasts at screen position, branches on hit type. */
	FMoveTarget ProcessPrimaryInteraction(AActor* HitActor, FVector HitLocation);
	
	FMoveTarget GetPendingMovementTarget() const {	return PendingMovementTarget; }
 	
	/** Distance within which a corpse can be looted on click. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float LootRange = 250.0f;

	/** Echo-poll rate while auto-path is waiting to arrive at a corpse. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float LootArrivalPollInterval = 0.2f;
	
	/** Clears the pending-loot state and stops the arrival timer. */
	void ClearPendingLoot();
private:
	/** Loots Corpse now if the pawn is in range; otherwise starts auto-path + arrival poll. */
	void TryLootCorpse(AOnsetCorpse* Corpse);

	/** Echo poll while pathing: loots the pending corpse once the pawn arrives. */
	void OnLootArrivalTick();

	/** Transfers loot to the pawn's inventory, marks/destroys the corpse, and fires the UI trigger. */
	void LootCorpse(AOnsetCorpse* Corpse, APawn* Pawn);

	UPROPERTY()
	TObjectPtr<UTargetingComponent> TargetingComponent;
	
	FMoveTarget PendingMovementTarget = {};
	
	/** The corpse we are auto-pathing to (server-side only). */
	TWeakObjectPtr<AOnsetCorpse> PendingLootCorpse;

	FTimerHandle LootArrivalTimerHandle;
	
};
