// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DetourCrowdAIController.h"
#include "Enemy/Profile/PerceptionProfile.h"
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
	bool InUse() const { return bInUse; }
	// --- Components ---

	/** StateTree execution component. Started on possess, stopped on pool return. */
	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;

	/** Push a profile to this controller — sets StateTree asset and configures perception. */
	UFUNCTION(BlueprintCallable, Category = "AI")
	void ApplyAIProfile(UAIProfile* Profile);
	UFUNCTION(BlueprintCallable, Category = "AI")
	const UAIProfile* GetAIProfile() const { return AIProfile; } 
	void ApplyPerceptionProfile(const UPerceptionProfile* Profile);

	// --- Targeting ---

	/** Current cached Targeting Component set in OnPossess, cleared on UnPossess. */
	UPROPERTY()
	TObjectPtr<UTargetingComponent> TargetingComponent;
	
	FVector HeardNoiseLocation = FVector::ZeroVector;                                                               
	TWeakObjectPtr<AActor> HeardNoiseInstigator;                                                                    
	bool bHasPendingNoise = false;                                                                                  
	float LastNoiseHeardTime = 0.0f;        

	// --- AI LOD ---

	/** Cached sight range for LOD tier calculation (set in ApplyPerceptionProfile). */
	float CachedSightRange = 0.0f;
	/** Cached hearing range for LOD tier calculation. */
	float CachedHearingRange = 0.0f;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

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
	void UpdateLodTier();
	int32 LodTickCounter = 0;

	bool bInUse = false;
	UPROPERTY()
	UAIProfile* AIProfile;
};
