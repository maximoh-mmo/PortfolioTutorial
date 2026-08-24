// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/OnsetPlayerAIController.h"
#include "StateTree.h"
#include "TimerManager.h"
#include "Components/StateTreeAIComponent.h"
#include "Core/TargetingComponent.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"
#include "GAS/OnsetAttributeSet.h"
#include "Player/OnsetPlayerCharacter.h"
#include "Player/OnsetPlayerController.h"

#include "Subsystem/OnsetPlayerDataSubsystem.h"


// Sets default values
AOnsetPlayerAIController::AOnsetPlayerAIController()
{
	bStartAILogicOnPossess = true;
	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeComponent"));
	StateTreeComponent->SetComponentTickEnabled(false);
	// Possession must never implicitly start logic using stale instance data from a
	// previous enable/disable cycle - that zombie-start blocked SetStateTree on the
	// next takeover ("running instance") and left autoplay fighting ghosts.
	// StartStateTree() is the sole owner of the start sequence.
	StateTreeComponent->SetStartLogicAutomatically(false);
	StateTree = LoadObject<UStateTree>(nullptr, TEXT("/Game/AI/PlayerAutoCombat.PlayerAutoCombat"));
	
	// Possession must never implicitly start logic using stale instance data from a
	// previous enable/disable cycle - that zombie-start blocked SetStateTree on the
	// next takeover ("running instance") and left autoplay fighting ghosts.
	// StartStateTree() is the sole owner of the start sequence.
	StateTreeComponent->SetStartLogicAutomatically(false);
}

void AOnsetPlayerAIController::StartStateTree()
{
	if (!StateTree)
	{
		StateTree = LoadObject<UStateTree>(nullptr, TEXT("/Game/AI/PlayerAutoCombat.PlayerAutoCombat"));
	}
	if (StateTree)
	{
#if WITH_EDITOR
	
		if (!StateTree->IsReadyToRun())
		{
			StateTree->MarkAsModified(false);
			StateTree->CompileIfChanged();
		}
	
#endif
	
		if (StateTree->IsReadyToRun())
		{
			// Repeated enable/disable cycles may reach here while a previous
			// instance is still winding down; SetStateTree refuses changes on a
			// running component, so stop explicitly first.
			if (StateTreeComponent->IsRunning())
			{
				StateTreeComponent->StopLogic(TEXT("Restarting player StateTree"));
			}
			StateTreeComponent->SetStateTree(StateTree);
			StateTreeComponent->SetComponentTickEnabled(true);
			StateTreeComponent->StartLogic();
		}
	}
	else
	{
		UE_LOG(LogController, Error, TEXT("StateTree not ready. Open /Game/AI/PlayerAutoCombat and save it."));
	}
}

void AOnsetPlayerAIController::StopStateTree()
{
	if (IsValid(StateTreeComponent))
	{
		if (StateTreeComponent->IsRunning())
		{
			StateTreeComponent->StopLogic(TEXT("PlayerOverride"));
			StateTreeComponent->SetComponentTickEnabled(false);
		}
	}
}

void AOnsetPlayerAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (!HasAuthority()) return;

	// Heal any state a previous cycle may have left behind.
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	TargetingComponent = InPawn->FindComponentByClass<UTargetingComponent>();
	StartStateTree();
	UE_LOG(LogActor, Warning, TEXT("AOnsetPlayerAIController: Possessed player"));
}

void AOnsetPlayerAIController::IssueClickMove(const FVector& Destination, AOnsetPlayerController* OwningPC)
{
	ClickMoveOwner = OwningPC;

	// Coalesce rapid re-clicks: each MoveToLocation aborts the active path (velocity
	// reset) and issues a fresh async A* query, which reads as stutter when spammed.
	// Skip re-issues that are near-identical to the active goal within the throttle
	// window; genuine retargets always pass through immediately.
	const float Now = GetWorld()->GetTimeSeconds();
	const bool bThrottled = bPendingClickMove
		&& (Now - LastClickMoveTime) < ClickMoveMinInterval
		&& FVector::DistSquaredXY(Destination, LastClickMoveDestination) < FMath::Square(SameGoalThreshold);

	LastClickMoveDestination = Destination;
	if (bThrottled)
	{
		return;
	}
	LastClickMoveTime = Now;

	bPendingClickMove = true;

#if !UE_BUILD_SHIPPING
	UE_LOG(LogTemp, Warning, TEXT("[DiagWalk] IssueClickMove %s dest=%s"),
		*GetName(), *Destination.ToCompactString());
#endif

	// Combat brain off for the walk; StartStateTree() re-engages it on the next
	// autoplay cycle.
	StopStateTree();
	MoveToLocation(Destination);

#if !UE_BUILD_SHIPPING
	if (UPathFollowingComponent* PFC = GetPathFollowingComponent())
	{
		UE_LOG(LogTemp, Warning, TEXT("[DiagWalk] post-MoveTo status=%d path-valid=%d"),
			static_cast<int32>(PFC->GetStatus()),
			PFC->GetPath() ? 1 : 0);
	}
#endif
}

void AOnsetPlayerAIController::IssueClickMoveToActor(AActor* Goal, float AcceptanceRadius,
	AOnsetPlayerController* OwningPC)
{
	ClickMoveOwner = OwningPC;

	const float Now = GetWorld()->GetTimeSeconds();
	const bool bThrottled = bPendingClickMove
		&& ClickMoveGoalActor.IsValid() && ClickMoveGoalActor.Get() == Goal
		&& (Now - LastClickMoveTime) < ClickMoveMinInterval;

	LastClickMoveTime = Now;
	if (bThrottled)
	{
		return;
	}

	bPendingClickMove = true;
	ClickMoveGoalActor = Goal;

	// Combat brain off for the walk; StartStateTree() re-engages it on the next
	// autoplay cycle.
	StopStateTree();
	MoveToActor(Goal, AcceptanceRadius);
}

void AOnsetPlayerAIController::OnUnPossess()
{
	// Stop the tree explicitly and leave the component clean so the next
	// EnableAutoCombat starts from a known-stopped state (StartLogic is owned by
	// StartStateTree; automatic start-on-possess is disabled in the constructor).
	if (StateTreeComponent)
	{
		StateTreeComponent->StopLogic(TEXT("PlayerOverride"));
		StateTreeComponent->SetComponentTickEnabled(false);
	}
	ClearAbandonedTimeout();

	Super::OnUnPossess();
	UE_LOG(LogActor, Warning, TEXT("AOnsetPlayerAIController: Returned control to player"));
}

void AOnsetPlayerAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	if (!bPendingClickMove || Result.IsInterrupted())
	{
		return; // superseded by a newer request, or not a click-move
	}
	bPendingClickMove = false;
	ClearFocus(EAIFocusPriority::Gameplay);

	// Arrival hands the pawn back to the player: DisableAutoCombat re-possesses the
	// PlayerController and broadcasts the settings change that deselects the HUD toggle.
	if (AOnsetPlayerController* PC = ClickMoveOwner.Get())
	{
		ClickMoveOwner = nullptr;
		PC->DisableAutoCombat();
	}
}

void AOnsetPlayerAIController::AdoptAbandonedPawn(APawn* InPawn, const FString& Platform, const FString& PlatformID, int32 SlotIndex)
{
	if (!InPawn || !HasAuthority()) return;

	PendingAbandonedPawn = InPawn;
	CachedPlatform = Platform;
	CachedPlatformID = PlatformID;
	CachedSlotIndex = SlotIndex;

	// Defer possession a tick: the leaving PC is mid-Destroyed() right now, so
	// possessing its pawn immediately would trigger reentrant controller callbacks.
	// The despawn countdown is armed after a successful possession (see
	// PossessAbandonedPawn) so it can't be wiped by a re-possess side effect.
	GetWorldTimerManager().SetTimerForNextTick(this, &AOnsetPlayerAIController::PossessAbandonedPawn);

	UE_LOG(LogActor, Warning, TEXT("AOnsetPlayerAIController: adopted abandoned pawn %s (slot %d, %.1fs timeout)"),
		*GetNameSafe(InPawn), SlotIndex, AbandonedTimeoutSeconds);
}

void AOnsetPlayerAIController::PossessAbandonedPawn()
{
	APawn* AdoptedPawn = PendingAbandonedPawn;
	PendingAbandonedPawn = nullptr;

	if (!AdoptedPawn || !HasAuthority() || AdoptedPawn->IsActorBeingDestroyed())
	{
		return;
	}

	// OnPossess starts the StateTree.
	Possess(AdoptedPawn);

	// The pawn now has no PlayerState (owned by the destroyed PlayerController + AAIController
	// has none), so give it the stored identity for progression persistence (Leveling_System).
	if (AOnsetPlayerCharacter* PlayerChar = Cast<AOnsetPlayerCharacter>(AdoptedPawn))
	{
		PlayerChar->SetPersistIdentity(CachedPlatform, CachedPlatformID, CachedSlotIndex);
	}

	// Arm the despawn countdown now that possession succeeded. If the pawn dies in
	// combat first, OnUnPossess clears it.
	if (GetPawn() == AdoptedPawn)
	{
		GetWorldTimerManager().ClearTimer(AbandonedTimeoutHandle);
		GetWorldTimerManager().SetTimer(AbandonedTimeoutHandle, this,
			&AOnsetPlayerAIController::OnAbandonedTimeout, AbandonedTimeoutSeconds, false);
	}
}

void AOnsetPlayerAIController::OnAbandonedTimeout()
{
	if (APawn* AdoptedPawn = GetPawn())
	{
		// Save final state before despawning.
		AOnsetPlayerCharacter* PlayerChar = Cast<AOnsetPlayerCharacter>(AdoptedPawn);
		if (PlayerChar && CachedSlotIndex >= 0)
		{
			UOnsetPlayerDataSubsystem* DataSubsystem = GetWorld()->GetSubsystem<UOnsetPlayerDataSubsystem>();
			if (DataSubsystem)
			{
				FOnsetFullCharacterData CharData;
				CharData.SlotIndex = CachedSlotIndex;
				CharData.SavedPosition = PlayerChar->GetActorLocation();
				CharData.SavedRotationYaw = PlayerChar->GetActorRotation().Yaw;
				CharData.CurrentZone = GetWorld()->GetMapName();
				if (PlayerChar->AttributeSet)
				{
					CharData.SavedMaxHealth = PlayerChar->AttributeSet->GetMaxHealth();
				}
				CharData.InventoryJSON = TEXT("{}");
				CharData.EquipmentJSON = PlayerChar->SerializeEquipmentJSON();
				CharData.QuestsJSON = TEXT("{}");
				DataSubsystem->SaveCharacterPreservingIdentity(CachedPlatform, CachedPlatformID, CharData);
				UE_LOG(LogActor, Log, TEXT("AOnsetPlayerAIController: timeout save for slot %d"), CachedSlotIndex);
			}
		}

		UE_LOG(LogActor, Warning, TEXT("AOnsetPlayerAIController: abandoned-pawn timeout — despawning %s"), *GetNameSafe(AdoptedPawn));
		StopStateTree();
		AdoptedPawn->Destroy();
	}

	ClearAbandonedTimeout();
}

void AOnsetPlayerAIController::ClearAbandonedTimeout()
{
	GetWorldTimerManager().ClearTimer(AbandonedTimeoutHandle);
}