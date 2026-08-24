// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/OnsetPlayerAIController.h"
#include "StateTree.h"
#include "TimerManager.h"
#include "Components/StateTreeAIComponent.h"
#include "Core/TargetingComponent.h"
#include "GameFramework/Pawn.h"
#include "GAS/OnsetAttributeSet.h"
#include "Player/OnsetPlayerCharacter.h"
#include "Subsystem/OnsetPlayerDataSubsystem.h"


// Sets default values
AOnsetPlayerAIController::AOnsetPlayerAIController()
{
	bStartAILogicOnPossess = true;
	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeComponent"));
	StateTreeComponent->SetComponentTickEnabled(false);
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
	TargetingComponent = InPawn->FindComponentByClass<UTargetingComponent>();
	StartStateTree();
	UE_LOG(LogActor, Warning, TEXT("AOnsetPlayerAIController: Possessed player"));
}

void AOnsetPlayerAIController::OnUnPossess()
{
	StateTreeComponent->StopLogic(TEXT("PlayerOverride"));
	StateTreeComponent->SetComponentTickEnabled(false);
	ClearAbandonedTimeout();
	UE_LOG(LogActor, Warning, TEXT("AOnsetPlayerAIController: Returned control to player"));

	Super::OnUnPossess();
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