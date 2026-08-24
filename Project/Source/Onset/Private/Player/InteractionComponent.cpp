#include "Player/InteractionComponent.h"

#include "NavigationSystem.h"
#include "TimerManager.h"
#include "Core/OnsetBaseCharacter.h"
#include "Corpse/OnsetCorpse.h"
#include "Enemy/OnsetEnemy.h"
#include "Inventory/UOnsetInventoryComponent.h"
#include "Player/OnsetPlayerController.h"
#include "Core/TargetingComponent.h"
#include "Player/OnsetPlayerState.h"

UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FMoveTarget UInteractionComponent::ProcessPrimaryInteraction(AActor* HitActor, FVector HitLocation)
{
	AOnsetPlayerController* PlayerController = Cast<AOnsetPlayerController>(GetOwner());
	if (!PlayerController)
	{
		UE_LOG(LogGamepad, Log, TEXT("ProcessPrimaryInteraction: Owner is null or not OnsetPlayerController"));
		return {};
	}

	APawn* Pawn = PlayerController->GetPawn();
	TargetingComponent = Pawn ? Pawn->FindComponentByClass<UTargetingComponent>() : nullptr;

	PendingMovementTarget = {};

	// Corpse click → clear target, stop autoattack, loot now if in range, otherwise auto-path and loot on arrival.
	if (AOnsetCorpse* Corpse = Cast<AOnsetCorpse>(HitActor))
	{
		if (TargetingComponent)
		{
			TargetingComponent->ClearTarget();
		}
		PlayerController->StopAutoAttack();
		TryLootCorpse(Corpse);
		return PendingMovementTarget;
	}

	// Any non-corpse click cancels pending corpse loot
	ClearPendingLoot();

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation NavLoc;
	bool bIsHostile = false;
	
	if (HitActor && TargetingComponent)
	{
		const bool bEnemyTag = HitActor->ActorHasTag("Enemy");
		const bool bIsEnemy = HitActor->IsA<AOnsetEnemy>();
		const bool bPVPValid = TargetingComponent->IsActorTargetPVPValid(HitActor, Pawn);
		const bool bValidTarget = TargetingComponent->IsActorTargetValid(HitActor);

		if ((bEnemyTag || bIsEnemy || bPVPValid) && bValidTarget)
		{
			TargetingComponent->SetTarget(HitActor);
			PlayerController->StartAutoAttack();
			bIsHostile = true;
		}
	}
	
	const bool bProjected = NavSys && NavSys->ProjectPointToNavigation(HitLocation, NavLoc);
	if (bProjected)
	{
		PendingMovementTarget.Position = NavLoc.Location;
	}
	else
	{
		PendingMovementTarget.Position = HitLocation;
	}

	if (HitActor && bIsHostile)
	{
		PendingMovementTarget.Actor = HitActor;
	}

	if (!bIsHostile)
	{
		if (TargetingComponent)
		{
			TargetingComponent->ClearTarget();
		}
		PlayerController->StopAutoAttack();
	}

	return PendingMovementTarget;
}

void UInteractionComponent::TryLootCorpse(AOnsetCorpse* Corpse)
{
	if (!Corpse || Corpse->bLooted)
	{
		return;
	}

	AOnsetPlayerController* PlayerController = Cast<AOnsetPlayerController>(GetOwner());
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}

	if (FVector::Dist(Pawn->GetActorLocation(), Corpse->GetActorLocation()) > LootRange)
	{
		// Out of range: the owning client is walking here and will re-send on arrival.
		return;
	}

	// Out of range: path to the corpse and keep polling until we arrive.
	PendingLootCorpse = Corpse;
	PendingMovementTarget.Actor = Corpse;
	PendingMovementTarget.Position = Corpse->GetActorLocation();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			LootArrivalTimerHandle, this, &UInteractionComponent::OnLootArrivalTick, LootArrivalPollInterval, true);
	}
}

void UInteractionComponent::OnLootArrivalTick()
{
	AOnsetCorpse* Corpse = PendingLootCorpse.Get();
	if (!IsValid(Corpse) || Corpse->bLooted)
	{
		ClearPendingLoot();
		return;
	}
	TryLootCorpse(Corpse);
}

void UInteractionComponent::LootCorpse(AOnsetCorpse* Corpse, APawn* Pawn)
{
	if (!Corpse || Corpse->bLooted)
	{
		return;
	}

	UOnsetInventoryComponent* CorpseInventory = Corpse->InventoryComponent;
	UOnsetInventoryComponent* PawnInventory = Pawn->FindComponentByClass<UOnsetInventoryComponent>();
	if (!CorpseInventory || !PawnInventory)
	{
		return;
	}

	const TArray<FOnsetInventoryEntry> Loot = CorpseInventory->GetItems();
	if (Loot.Num() > 0)
	{
		PawnInventory->AddItems(Loot);
		if (AOnsetPlayerController* PlayerController = Cast<AOnsetPlayerController>(GetOwner()))
		{
			PlayerController->Client_ShowLootOverlay(Loot);
		}
	}

	Corpse->bLooted = true;
	Corpse->Destroy();

	UE_LOG(LogTemp, Log, TEXT("LootCorpse: %s looted %d item(s)"), *Pawn->GetName(), Loot.Num());
}

void UInteractionComponent::ClearPendingLoot()
{
	PendingLootCorpse.Reset();
	PendingMovementTarget.Position = FVector::ZeroVector;
	PendingMovementTarget.Actor = nullptr;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LootArrivalTimerHandle);
	}
}
