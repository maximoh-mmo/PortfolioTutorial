// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DetourCrowdAIController.h"
#include "OnsetAIController.generated.h"

class UStateTreeAIComponent;
class UTargetingComponent;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UAIProfile;

/** Controller for NPC and Player-AI pawns. Owns perception, state tree, and targeting. */
UCLASS()
class ONSET_API AOnsetAIController : public ADetourCrowdAIController
{
	GENERATED_BODY()

public:
	AOnsetAIController();

	UFUNCTION(BlueprintPure, Category = "AI")
	bool InUse() { return bInUse; }
	// --- Components ---

	/** StateTree execution component. Started on possess, stopped on pool return. */
	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;

	/** Push a profile to this controller — sets StateTree asset and configures perception. */
	UFUNCTION(BlueprintCallable, Category = "AI")
	void ApplyProfile(const UAIProfile* Profile);

	// --- Targeting ---

	/** Current target actor. Set by OnPerceptionUpdated for NPCs, by input for Player AI. */
	UPROPERTY()
	UTargetingComponent* TargetingComponent;
	
	// --- Pooling ---
	/** Resets the controller to base, non-active, state for pooling reuse. Called by pool manager on release. */
	UFUNCTION(BlueprintCallable, Category = "Pooling")
	void ResetForPool();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void BeginPlay() override;

	// --- Perception ---

	/** Called when any perceived actor changes state (enters/leaves sight or hearing range). */
	UFUNCTION()
	void OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors);

	/** Sight sense config, configured per AIProfile. */
	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	/** Hearing sense config, configured per AIProfile. */
	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;
	
private:
	bool bInUse;
};
