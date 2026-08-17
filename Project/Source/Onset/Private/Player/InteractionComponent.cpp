#include "Player/InteractionComponent.h"
#include "Corpse/OnsetCorpse.h"
#include "Inventory/UOnsetInventoryComponent.h"
#include "NavigationSystem.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Player/OnsetPlayerController.h"
#include "Core/TargetingComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"

class UNavigationSystemV1;

UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInteractionComponent::ProcessPrimaryInteraction(AActor* HitActor, FVector HitLocation)
{
	AOnsetPlayerController* PlayerController = Cast<AOnsetPlayerController>(GetOwner());
	if (!PlayerController)
	{
		UE_LOG(LogGamepad, Log, TEXT("ProcessPrimaryInteraction: Owner is null or not OnsetPlayerController"));
		return;
	}

	// Corpse click → loot now if in range, otherwise auto-path and loot on arrival.
	if (AOnsetCorpse* Corpse = Cast<AOnsetCorpse>(HitActor))
	{
		TryLootCorpse(Corpse);
		return;
	}

	if (!TargetingComponent)
	{
		if (APawn* Pawn = PlayerController->GetPawn())
		{
			TargetingComponent = Pawn->FindComponentByClass<UTargetingComponent>();
			if (!TargetingComponent) return;
		}
		else
		{
			return;
		}
	}
	
	PendingMoveTarget = FVector::ZeroVector;
	
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation NavLoc;
	bool bIsHostile = false;
	
	if (HitActor)
	{
		bool bEnemyTag = HitActor->ActorHasTag("Enemy");
		bool bPVPValid = TargetingComponent->IsActorTargetPVPValid(HitActor, PlayerController->GetPawn());
		if (bEnemyTag || bPVPValid)
		{
			TargetingComponent->SetTarget(HitActor);
			PlayerController->StartAutoAttack();
			bIsHostile = true;
		}
	}
	
	bool bProjected = NavSys && NavSys->ProjectPointToNavigation(HitLocation, NavLoc);
	if (bProjected)
	{
		PendingMoveTarget = NavLoc.Location;
	}
	else if (HitActor)
	{
		PendingMoveTarget = HitActor->GetActorLocation();
	}
	
	if (!bIsHostile)
	{
		TargetingComponent->ClearTarget();
		PlayerController->StopAutoAttack();
	}
}

void UInteractionComponent::TryLootCorpse(AOnsetCorpse* Corpse)
{
	if (!Corpse || Corpse->bLooted)
	{
		ClearPendingLoot();
		return;
	}

	AOnsetPlayerController* PlayerController = Cast<AOnsetPlayerController>(GetOwner());
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}

	const float Distance = FVector::Dist(Pawn->GetActorLocation(), Corpse->GetActorLocation());
	if (Distance <= LootRange)
	{
		LootCorpse(Corpse, Pawn);
		return;
	}

	// Out of range: path to the corpse and keep polling until we arrive.
	PendingLootCorpse = Corpse;
	PendingMoveTarget = Corpse->GetActorLocation();
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
		ClearPendingLoot();
		return;
	}

	UOnsetInventoryComponent* CorpseInventory = Corpse->InventoryComponent;
	UOnsetInventoryComponent* PawnInventory = Pawn->FindComponentByClass<UOnsetInventoryComponent>();
	if (!CorpseInventory || !PawnInventory)
	{
		ClearPendingLoot();
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
	ClearPendingLoot();

	UE_LOG(LogTemp, Log, TEXT("LootCorpse: %s looted %d item(s)"), *Pawn->GetName(), Loot.Num());
}

void UInteractionComponent::ClearPendingLoot()
{
	PendingLootCorpse.Reset();
	PendingMoveTarget = FVector::ZeroVector;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LootArrivalTimerHandle);
	}
}
