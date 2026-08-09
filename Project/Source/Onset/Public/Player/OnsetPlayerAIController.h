// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "OnsetPlayerAIController.generated.h"

class UStateTree;
class UStateTreeAIComponent;
class UTargetingComponent;
class APawn;

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
	
	void StartStateTree();
	void StopStateTree();
protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	
private:
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
