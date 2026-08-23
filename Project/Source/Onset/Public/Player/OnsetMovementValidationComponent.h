// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OnsetMovementValidationComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOnsetValidation, Warning, All);

/**
 * Server-side movement validation for client-authoritative player pawns.
 *
 * Tiered checks, no route computation:
 *  T1 - delta budget: displacement over the sample window must fit within
 *       MaxWalkSpeed x SpeedToleranceMultiplier x elapsed (horizontal plane).
 *  T2 - through-wall probe: line trace from last accepted position to the
 *       incoming position; blocked by world geometry = violation.
 *  T3 - escalation stub: navmesh reachability spot-check on repeated suspicion.
 *
 * Violations correct the pawn to its last accepted server position and are
 * logged to the game-server log only (never replicated to clients). Repeated
 * violations inside SuspicionWindowSeconds raise a single FLAGGED announcement.
 *
 * Runs exclusively on the server and only while the owning pawn is genuinely
 * player-controlled - autoplay possession is server-simulated and skipped by
 * construction, so legitimate AI steering can never be flagged.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ONSET_API UOnsetMovementValidationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOnsetMovementValidationComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/** Seconds between replicated-position samples. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement Validation")
	float SamplePeriod = 0.25f;

	/** Multiplier over the pawn's current max speed allowed per sample window. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement Validation")
	float SpeedToleranceMultiplier = 1.5f;

	/** Seconds between opportunistic through-wall probes for fast-but-plausible moves. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement Validation")
	float WallProbeCadence = 1.0f;

	/** Single-sample displacement that always demands a wall probe. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement Validation")
	float MaxTeleportDelta = 500.0f;

	/** Violations within SuspicionWindowSeconds before the pawn is announced as flagged. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement Validation")
	int32 SuspicionThreshold = 5;

	/** Rolling window for the suspicion threshold. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement Validation")
	float SuspicionWindowSeconds = 10.0f;

	/** Server-side: treat the current position as trusted (teleports, respawns, spawns). */
	void SnapToCurrentPosition();

	/** Server-side: suppress speed-budget checks until now + Duration (knockbacks, dashes). */
	void GrantMovementBurstExemption(float Duration);

	/** Cumulative violation count since spawn (debug/inspection; server-only). */
	int32 GetTotalViolations() const { return TotalViolations; }

protected:
	virtual void BeginPlay() override;

private:
	/** True when the incoming move violates a rule; applies the correction. */
	bool EvaluateMove(APawn* Pawn, const FVector& FromPos, const FVector& ToPos, float Elapsed);

	void RegisterViolation(APawn* Pawn, const FString& Reason, const FVector& CorrectedTo);

	/** Last server-trusted position. */
	FVector LastValidPosition = FVector::ZeroVector;

	float LastSampleTime = -1.0f;
	float NextWallProbeTime = 0.0f;
	float ExemptionUntilTime = 0.0f;

	int32 TotalViolations = 0;
	TArray<float> RecentViolationTimes;
	bool bFlaggedThisWindow = false;

	/** Guards the very first sample (spawn placement is trusted implicitly). */
	bool bHasAcceptedSample = false;

	/** Ownership-transition detection: regaining player control after autoplay/death
	 *  implies a legitimate server-side teleport - positions re-trust automatically. */
	bool bWasPlayerControlled = false;
};
