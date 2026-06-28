#include "Player/InteractionComponent.h"
#include "NavigationSystem.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Player/OnsetPlayerController.h"
#include "Core/TargetingComponent.h"

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
