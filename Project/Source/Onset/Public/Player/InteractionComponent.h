// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"


class AOnsetCorpse;
class AOnsetPlayerController;
class UTargetingComponent;

/**
 * Server-side primary-interaction resolution: enemy targeting and corpse looting.
 * Traversal is owned by the client (see AOnsetPlayerController local move layer);
 * this component only applies gameplay-authoritative results of a click.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ONSET_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionComponent();

	/** Server-side click resolution: enemy -> SetTarget(+auto-attack); corpse ->
	 *  ClearTarget + loot-if-in-range (client drives proximity); floor -> ClearTarget. */
	void ProcessPrimaryInteraction(AActor* HitActor, FVector HitLocation);

	/** Distance within which a corpse can be looted on click. Must stay in sync with
	 *  the owning client's pursuit stop logic (client polls this value locally). */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float LootRange = 250.0f;

private:
	/** Loots Corpse if the pawn is within LootRange; otherwise no-op (client re-sends on arrival). */
	void TryLootCorpse(AOnsetCorpse* Corpse);

	/** Transfers loot to the pawn's inventory, marks/destroys the corpse, and fires the UI trigger. */
	void LootCorpse(AOnsetCorpse* Corpse, APawn* Pawn);

	UPROPERTY()
	TObjectPtr<UTargetingComponent> TargetingComponent;
};
