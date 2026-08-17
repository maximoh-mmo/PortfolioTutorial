// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OnsetCorpse.generated.h"

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

	/** True once a player has looted this corpse. Server-authoritative loot guard. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Loot")
	bool bLooted = false;
};
	