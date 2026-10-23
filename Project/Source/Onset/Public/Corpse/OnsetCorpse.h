// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MHInteractionTypes.h"
#include "OnsetCorpse.generated.h"

class UMHInteractableComponent;
class UOnsetInventoryComponent;
class UStaticMeshComponent;

/** A corpse actor with a static mesh, spawned and recycled by UOnsetCorpseSubsystem. */
UCLASS()
class ONSET_API AOnsetCorpse : public AActor
{
	GENERATED_BODY()

public:
	AOnsetCorpse();

	/** Static mesh component for the corpse visual. */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/**
	 * Loot contents. Replicated to all clients (owner-only is disabled here) so
	 * any player can see the server-rolled loot; populated by the loot pass.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UOnsetInventoryComponent> InventoryComponent;

	/** Modular MH Framework interaction component for click-to-loot and proximity [E] Loot. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot|Interaction")
	TObjectPtr<UMHInteractableComponent> InteractableComponent;

	/** True once a player has looted this corpse. Server-authoritative loot guard. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Loot")
	bool bLooted = false;

	/** Transfers loot to InstigatorPawn's inventory, fires Client_ShowLootOverlay, and destroys the corpse. */
	bool PerformLoot(AActor* InstigatorPawn, AController* InstigatorController = nullptr);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	bool QueryCanInteract(const FMHInteractionContext& Context, FText& OutFailureReason) const;
	void HandleInteracted(const FMHInteractionContext& Context);
};
