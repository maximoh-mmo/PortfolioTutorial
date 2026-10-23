// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MHInteractionTypes.h"
#include "OnsetInteractableTrigger.generated.h"

class UBoxComponent;
class UMHInteractableComponent;
class USceneComponent;
class UStaticMeshComponent;

/**
 * Modular world trigger / switch / lever / pressure plate / shrine actor.
 * Supports explicit interaction (click / E key), automatic overlap triggering, or both via
 * UMHInteractableComponent, and can toggle/unlock linked AOnsetDoor actors or advance quest objectives.
 */
UCLASS(Blueprintable)
class ONSET_API AOnsetInteractableTrigger : public AActor
{
	GENERATED_BODY()

public:
	AOnsetInteractableTrigger();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trigger")
	TObjectPtr<USceneComponent> RootScene;

	/** Optional visual mesh for levers, switches, pressure plates, or shrines. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trigger")
	TObjectPtr<UStaticMeshComponent> TriggerMesh;

	/** Overlap volume bound to InteractableComponent for OverlapOnly / ExplicitAndOverlap trigger modes. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trigger")
	TObjectPtr<UBoxComponent> OverlapVolume;

	/** Modular MH Framework interaction component. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trigger|Interaction")
	TObjectPtr<UMHInteractableComponent> InteractableComponent;

	/** Actors in the level (e.g. AOnsetDoor instances) that are toggled when this trigger fires. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Trigger|Links")
	TArray<TObjectPtr<AActor>> LinkedActors;

	/** If true, linked AOnsetDoor actors will be force-unlocked when toggled by this trigger. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger|Links")
	bool bUnlockLinkedDoors = true;

	/** If true, also reports a ReachLocation checkpoint at this trigger's location to UOnsetQuestComponent. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger|Quest")
	bool bReportReachLocation = false;

	/** Replicated toggle state (true after odd activations, false after even activations). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_IsActivated, Category = "Trigger")
	bool bIsActivated = false;

	/** Blueprint hook fired when the trigger state changes. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Trigger")
	void BP_OnTriggerStateChanged(bool bNowActivated);

	/** Blueprint hook fired on server authority when a player activates the trigger. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Trigger")
	void BP_OnTriggeredByPawn(AActor* InstigatorPawn, bool bByOverlap);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_IsActivated();

private:
	void HandleInteracted(const FMHInteractionContext& Context);
};
