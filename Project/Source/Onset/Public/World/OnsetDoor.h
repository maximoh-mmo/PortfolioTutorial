// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "MHInteractionTypes.h"
#include "OnsetDoor.generated.h"

class UMHInteractableComponent;
class USceneComponent;
class UStaticMeshComponent;

/**
 * Modular interactable door actor supporting rotational swing and/or linear slide,
 * key-item or quest-completion lock requirements, remote trigger activation, and replication.
 */
UCLASS(Blueprintable)
class ONSET_API AOnsetDoor : public AActor
{
	GENERATED_BODY()

public:
	AOnsetDoor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<USceneComponent> RootScene;

	/** Hinge/pivot scene component animated when the door opens or closes. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<USceneComponent> DoorPivot;

	/** Visual/collision mesh attached to DoorPivot. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	/** Modular MH Framework interaction component. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door|Interaction")
	TObjectPtr<UMHInteractableComponent> InteractableComponent;

	/** Whether the door is currently open (replicated to all clients). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_IsOpen, Category = "Door")
	bool bIsOpen = false;

	/** Whether the door is currently locked. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Door|Lock")
	bool bIsLocked = false;

	/** Optional item RowName required in the player's inventory to unlock this door on interaction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Lock")
	FName RequiredKeyItemRow = NAME_None;

	/** If true, unlocking the door consumes 1 of RequiredKeyItemRow from the player's inventory. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Lock")
	bool bConsumeKeyOnUnlock = false;

	/** Optional quest RowName that must be completed to unlock or open this door. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Lock")
	FName RequiredQuestComplete = NAME_None;

	/** Relative rotation offset applied to DoorPivot when open (e.g. Yaw = 90 for a swinging door). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Motion")
	FRotator OpenRotationOffset = FRotator(0.0f, 90.0f, 0.0f);

	/** Relative location offset applied to DoorPivot when open (e.g. Z = 250 for a portcullis/sliding gate). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Motion")
	FVector OpenLocationOffset = FVector::ZeroVector;

	/** Speed at which the door interpolates between open and closed states (1 / duration in seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Motion", meta = (ClampMin = "0.1"))
	float OpenCloseSpeed = 2.5f;

	/** Optional MHAudio event triggered when the door begins opening. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Audio")
	FGameplayTag OpenAudioEvent;

	/** Optional MHAudio event triggered when the door begins closing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Audio")
	FGameplayTag CloseAudioEvent;

	/** Optional MHAudio event triggered when an interaction attempt fails because the door is locked. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Audio")
	FGameplayTag LockedAudioEvent;

	/** Opens or closes the door on server authority. */
	UFUNCTION(BlueprintCallable, Category = "Door")
	void SetDoorOpen(bool bNewOpen);

	/** Toggles the door open/closed state on server authority (also unlocks if called by a linked trigger). */
	UFUNCTION(BlueprintCallable, Category = "Door")
	void ToggleDoor(bool bForceUnlock = false);

	/** Unlocks the door on server authority. */
	UFUNCTION(BlueprintCallable, Category = "Door")
	void UnlockDoor();

	/** Blueprint hook fired when the door state changes. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Door")
	void BP_OnDoorStateChanged(bool bNowOpen);

	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_IsOpen();

private:
	bool CanPawnUnlockDoor(const AActor* InstigatorPawn, FText& OutFailureReason) const;
	bool QueryCanInteract(const FMHInteractionContext& Context, FText& OutFailureReason) const;
	void ModifyInteractionDefinition(FMHInteractionDefinition& InOutDefinition);
	void HandleInteracted(const FMHInteractionContext& Context);
	void UpdateDoorTransform(float Alpha);

	FVector InitialPivotLocation = FVector::ZeroVector;
	FRotator InitialPivotRotation = FRotator::ZeroRotator;
	float CurrentOpenAlpha = 0.0f;
};
