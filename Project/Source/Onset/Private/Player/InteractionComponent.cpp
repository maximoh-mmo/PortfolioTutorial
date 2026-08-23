#include "Player/InteractionComponent.h"

#include "Core/OnsetBaseCharacter.h"
#include "Corpse/OnsetCorpse.h"
#include "Inventory/UOnsetInventoryComponent.h"
#include "Player/OnsetPlayerController.h"
#include "Core/TargetingComponent.h"
#include "Player/OnsetPlayerState.h"

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

	// Lazily resolve the pawn's targeting component (shared by every branch).
	if (!TargetingComponent)
	{
		if (APawn* Pawn = PlayerController->GetPawn())
		{
			TargetingComponent = Pawn->FindComponentByClass<UTargetingComponent>();
			if (!TargetingComponent)
			{
				return;
			}
		}
		else
		{
			return;
		}
	}

	// Corpse click: clear target and loot only when already in range - the owning
	// client drives traversal and re-sends this request once it arrives.
	if (AOnsetCorpse* Corpse = Cast<AOnsetCorpse>(HitActor))
	{
		TargetingComponent->ClearTarget();
		PlayerController->StopAutoAttack();
		TryLootCorpse(Corpse);
		return;
	}
	
	bool bIsHostile = false;
	if (HitActor)
	{
		const bool bEnemyTag = HitActor->ActorHasTag("Enemy");
		const bool bPVPValid = TargetingComponent->IsActorTargetPVPValid(HitActor, PlayerController->GetPawn());
		if (bEnemyTag || bPVPValid)
		{
			TargetingComponent->SetTarget(HitActor);
			PlayerController->StartAutoAttack();
			bIsHostile = true;
		}
	}

	// Floor click (or invalid hostile): drop the target.
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

	LootCorpse(Corpse, Pawn);
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
