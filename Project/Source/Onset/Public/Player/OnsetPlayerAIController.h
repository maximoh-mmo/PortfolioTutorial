// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "OnsetPlayerAIController.generated.h"

class UStateTree;
class UStateTreeAIComponent;
class UTargetingComponent;
class APawn;
class AOnsetPlayerController;

UCLASS()
class ONSET_API AOnsetPlayerAIController : public AAIController
{
	GENERATED_BODY()

public:
	AOnsetPlayerAIController();
	
	/** StateTree execution component. Started on possess, stopped on unpossess. */
	UPROPERTY(VisibleAnywhere, Category = "Auto Combat")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;
	
	/** Maximum distance ratio for movement destination acceptance. */
	UPROPERTY(EditAnywhere, Category = "Auto Combat")
	float MaxDistance = 0.5f;

	/** Aggressiveness bias affecting target selection and engagement decisions. */
	UPROPERTY(EditAnywhere, Category = "Auto Combat")
	float Aggression = 0.5f;

	/** Stores the current targeting component via OnPossess, clears on UnPossess. */          
	UTargetingComponent* GetTargetingComponent() const { return TargetingComponent; }
	
	UPROPERTY(EditDefaultsOnly, Category = "Auto Combat Ranges")
	float MinLeash = 800.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Auto Combat Ranges")
	float MaxLeash = 8000.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Auto Combat Ranges")
	float MinAcquire = 500.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Auto Combat Ranges")
	float MaxAcquire = 5000.0f;

	/** Seconds an abandoned (disconnected) player's pawn keeps auto-combating before it is despawned. */
	UPROPERTY(EditDefaultsOnly, Category = "Auto Combat")
	float AbandonedTimeoutSeconds = 60.0f;

	/**
	 * Adopts a leaving player's pawn so it keeps fighting while the player is offline.
	 * Possession is deferred a tick to avoid reentrancy during controller destruction.
	 */
	void AdoptAbandonedPawn(APawn* InPawn, const FString& Platform, const FString& PlatformID, int32 SlotIndex);

	/** Cached account identity of the adopted pawn (for the timeout save + pawn persistence). */
	FString GetCachedPlatform() const { return CachedPlatform; }
	FString GetCachedPlatformID() const { return CachedPlatformID; }
	int32 GetCachedSlotIndex() const { return CachedSlotIndex; }
	
	void StartStateTree();
	void StopStateTree();

	/**
	 * Player click-to-move: drives the pawn to Destination via navmesh pathing with the
	 * combat brain (StateTree) stopped. On arrival, control is handed back to OwningPC
	 * (DisableAutoCombat), which also deselects the HUD autoplay toggle. Rapid re-issues
	 * coalesce to avoid abort/repath stutter.
	 */
	void IssueClickMove(const FVector& Destination, AOnsetPlayerController* OwningPC);

	/** Actor-goal variant: PathFollowing tracks the (possibly moving) goal natively.
	 *  Used for enemy/corpse clicks so pursuit follows instead of a frozen snapshot. */
	void IssueClickMoveToActor(AActor* Goal, float AcceptanceRadius, AOnsetPlayerController* OwningPC);

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

private:
	/** Player controller awaiting pawn hand-back after a click-move completes. */
	TWeakObjectPtr<AOnsetPlayerController> ClickMoveOwner;

	/** True while a click-move issued via IssueClickMove is in flight. */
	bool bPendingClickMove = false;

	/** Min seconds between path re-issues for near-identical goals (anti-stutter). */
	UPROPERTY(EditDefaultsOnly, Category = "Auto Combat")
	float ClickMoveMinInterval = 0.2f;

	/** Distance considered "same goal" during the re-issue throttle window. */
	UPROPERTY(EditDefaultsOnly, Category = "Auto Combat")
	float SameGoalThreshold = 60.0f;

	FVector LastClickMoveDestination = FVector::ZeroVector;
	float LastClickMoveTime = -1000.0f;

	/** Actor goal of the in-flight click-move (enemy/corpse pursuit). */
	TWeakObjectPtr<AActor> ClickMoveGoalActor;

	/** Deferred possession + state tree start for an adopted pawn. */
	void PossessAbandonedPawn();

	/** Saves the adopted pawn's final state and despawns it after the timeout. */
	void OnAbandonedTimeout();

	/** Clears the abandoned-timeout timer when the pawn is destroyed early (e.g. killed in combat). */
	void ClearAbandonedTimeout();

	UPROPERTY()
	TObjectPtr<UTargetingComponent> TargetingComponent;
	
	UPROPERTY()
	TObjectPtr<UStateTree> StateTree;

	/** Pawn waiting to be possessed on the next tick (adopted from a leaving player). */
	UPROPERTY()
	TObjectPtr<APawn> PendingAbandonedPawn;

	/** Credentials captured from the leaving player's PlayerState, used for the timeout save. */
	FString CachedPlatform;
	FString CachedPlatformID;
	int32 CachedSlotIndex = -1;


	FTimerHandle AbandonedTimeoutHandle;
};
