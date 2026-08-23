#include "Player/OnsetMovementValidationComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/MovementComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogOnsetValidation);

UOnsetMovementValidationComponent::UOnsetMovementValidationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UOnsetMovementValidationComponent::BeginPlay()
{
	Super::BeginPlay();
	// Component exists only on player characters; nothing to validate on clients.
	SetComponentTickEnabled(GetOwner() && GetOwner()->GetNetMode() != NM_Client);

#if !UE_BUILD_SHIPPING
	if (GetOwner())
	{
		UE_LOG(LogOnsetValidation, Log, TEXT("[MoveValidation] active on %s (server tick)"), *GetOwner()->GetName());
	}
#endif
}

void UOnsetMovementValidationComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn)
	{
		return;
	}

	// Ownership-transition guard: while autoplay (or death) holds the pawn, movement
	// is server-simulated and trusted by construction. Regaining player control
	// implies a legitimate server-side placement - re-trust without sampling.
	const bool bPlayerControlled = Pawn->IsPlayerControlled();
	if (!bPlayerControlled)
	{
		bWasPlayerControlled = false;
		return; // server-simulated (autoplay/AI) movement is trusted by construction
	}
	if (!bWasPlayerControlled)
	{
		bWasPlayerControlled = true;
		SnapToCurrentPosition();
		UE_LOG(LogOnsetValidation, Log,
			TEXT("[MoveValidation] %s player control regained - position snapped"), *Pawn->GetName());
		return;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float Now = World->GetTimeSeconds();
	if (LastSampleTime < 0.0f)
	{
		LastSampleTime = Now;
		return;
	}
	const float Elapsed = Now - LastSampleTime;
	if (Elapsed < SamplePeriod)
	{
		return;
	}

	const FVector CurrentPos = Pawn->GetActorLocation();

	// Teleports/respawns are declared trusted via SnapToCurrentPosition.
	if (!bHasAcceptedSample)
	{
		LastValidPosition = CurrentPos;
		bHasAcceptedSample = true;
		LastSampleTime = Now;
		return;
	}

	if (Now < ExemptionUntilTime)
	{
		LastValidPosition = CurrentPos;
		LastSampleTime = Now;
		return;
	}

	const bool bViolated = EvaluateMove(Pawn, LastValidPosition, CurrentPos, Elapsed);
	if (bViolated)
	{
		RegisterViolation(Pawn, TEXT("delta/wall"), LastValidPosition);
		Pawn->SetActorLocation(LastValidPosition);
		// Keep LastValidPosition: the correction is the accepted state.
	}
	else
	{
		LastValidPosition = CurrentPos;
	}
	LastSampleTime = Now;

	// Opportunistic wall probe for moves that fit the speed budget but might clip
	// geometry on a straight line between samples.
	if (FVector::Dist(LastValidPosition, CurrentPos) > MaxTeleportDelta * 0.5f && Now >= NextWallProbeTime)
	{
		NextWallProbeTime = Now + WallProbeCadence;
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(OnsetMoveValidation), false, Pawn);
		if (World->LineTraceSingleByChannel(Hit, LastValidPosition, CurrentPos, ECC_Visibility, Params))
		{
			UE_LOG(LogOnsetValidation, Warning,
				TEXT("[MoveValidation] %s crossed blocking geometry between samples (%.0f uu)"),
				*Pawn->GetName(), FVector::Dist(LastValidPosition, CurrentPos));
			Pawn->SetActorLocation(LastValidPosition);
			RegisterViolation(Pawn, TEXT("through-wall"), LastValidPosition);
		}
	}
}

bool UOnsetMovementValidationComponent::EvaluateMove(APawn* Pawn, const FVector& FromPos,
	const FVector& ToPos, float Elapsed)
{
	const UMovementComponent* MoveComp = Pawn->GetMovementComponent();
	const float MaxSpeed = MoveComp ? MoveComp->GetMaxSpeed() : 600.0f;

	// Horizontal-plane budget: vertical displacement is dominated by gravity/falling
	// and would produce false positives against a flat speed budget.
	FVector Delta = ToPos - FromPos;
	Delta.Z = 0.0f;

	const float Budget = FMath::Max(MaxSpeed, 1.0f) * SpeedToleranceMultiplier
		* FMath::Max(Elapsed, KINDA_SMALL_NUMBER);

	const bool bSpeedViolation = Delta.SizeSquared() > FMath::Square(Budget);
	const bool bTeleportSized = FVector::Dist(FromPos, ToPos) > MaxTeleportDelta;

	if (!bSpeedViolation && !bTeleportSized)
	{
		return false;
	}

	// Confirm with a through-wall probe; an unblocked straight corridor at super speed
	// still fails the budget above, while blocked corridors fail here regardless.
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(OnsetMoveValidation), false, Pawn);
	if (GetWorld()->LineTraceSingleByChannel(Hit, FromPos, ToPos, ECC_Visibility, Params))
	{
		UE_LOG(LogOnsetValidation, Verbose,
			TEXT("[MoveValidation] %s move rejected: dist=%.0f budget=%.0f teleport=%d blocked-by=%s"),
			*Pawn->GetName(), FVector::Dist(FromPos, ToPos), Budget,
			static_cast<int32>(bTeleportSized), *GetNameSafe(Hit.GetActor()));
		return true;
	}

	// Unblocked: allow (e.g. legitimate falls/launches exceed horizontal budget rarely).
	return bSpeedViolation && bTeleportSized;
}

void UOnsetMovementValidationComponent::RegisterViolation(APawn* Pawn, const FString& Reason,
	const FVector& CorrectedTo)
{
	++TotalViolations;
	const UWorld* World = GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.0f;

	RecentViolationTimes.Add(Now);
	while (RecentViolationTimes.Num() > 0 && Now - RecentViolationTimes[0] > SuspicionWindowSeconds)
	{
		RecentViolationTimes.RemoveAt(0);
	}

	UE_LOG(LogOnsetValidation, Warning,
		TEXT("[MoveValidation] %s violation #%d (%s) - corrected to %s | recent=%d/%d in %.0fs%s"),
		*Pawn->GetName(), TotalViolations, *Reason, *CorrectedTo.ToCompactString(),
		RecentViolationTimes.Num(), SuspicionThreshold, SuspicionWindowSeconds,
		RecentViolationTimes.Num() >= SuspicionThreshold && !bFlaggedThisWindow ? TEXT(" ** FLAGGED **") : TEXT(""));

	if (RecentViolationTimes.Num() >= SuspicionThreshold)
	{
		if (!bFlaggedThisWindow)
		{
			bFlaggedThisWindow = true;
			UE_LOG(LogOnsetValidation, Error,
				TEXT("[MoveValidation] *** %s FLAGGED as suspicious mover (%d violations in %.0fs) ***"),
				*Pawn->GetName(), RecentViolationTimes.Num(), SuspicionWindowSeconds);
		}
	}
	else if (bFlaggedThisWindow)
	{
		// Window drained below threshold - re-arm for future detection.
		bFlaggedThisWindow = false;
	}
}

void UOnsetMovementValidationComponent::SnapToCurrentPosition()
{
	if (const APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		LastValidPosition = Pawn->GetActorLocation();
		bHasAcceptedSample = true;
		RecentViolationTimes.Reset();
		bFlaggedThisWindow = false;
	}
}

void UOnsetMovementValidationComponent::GrantMovementBurstExemption(float Duration)
{
	if (const UWorld* World = GetWorld())
	{
		ExemptionUntilTime = World->GetTimeSeconds() + FMath::Max(0.0f, Duration);
	}
}
