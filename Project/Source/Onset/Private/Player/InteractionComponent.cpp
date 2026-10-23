#include "Player/InteractionComponent.h"

#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "Core/OnsetBaseCharacter.h"
#include "Core/TargetingComponent.h"
#include "Corpse/OnsetCorpse.h"
#include "Enemy/OnsetEnemy.h"
#include "IMHInteractable.h"
#include "Player/OnsetPlayerController.h"
#include "UMHInteractionLibrary.h"

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

	// 1. Check if HitActor is an MHInteraction interactable (NPC, Door, Switch/Trigger, Corpse, etc.)
	if (HitActor && UMHInteractionLibrary::IsInteractableActor(HitActor))
	{
		FMHInteractionDefinition Def;
		UMHInteractionLibrary::GetInteractionDefinition(HitActor, Def);

		if (Def.TriggerMode != EMHInteractionTriggerMode::OverlapOnly)
		{
			if (TargetingComponent)
			{
				TargetingComponent->ClearTarget();
			}
			PlayerController->StopAutoAttack();
			TryInteractWithActor(HitActor);
			return PendingMovementTarget;
		}
	}

	// Any non-interactable click cancels pending walk-up interaction and closes open dialogue
	ClearPendingInteraction();
	PlayerController->CloseActiveDialogue();

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
		PendingMovementTarget.AcceptanceRadius = 200.0f;
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

void UInteractionComponent::TryInteractWithActor(AActor* TargetActor)
{
	if (!IsValid(TargetActor) || !UMHInteractionLibrary::IsInteractableActor(TargetActor))
	{
		ClearPendingInteraction();
		return;
	}

	AOnsetPlayerController* PlayerController = Cast<AOnsetPlayerController>(GetOwner());
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}

	FMHInteractionDefinition Def;
	UMHInteractionLibrary::GetInteractionDefinition(TargetActor, Def);

	FMHInteractionContext Context;
	Context.InstigatorActor = Pawn;
	Context.InstigatorController = PlayerController;
	Context.TargetActor = TargetActor;
	Context.bTriggeredByOverlap = false;

	// If already within interaction range, stop moving and request the interaction immediately.
	if (UMHInteractionLibrary::IsWithinInteractionRange(Pawn, TargetActor, 1.0f))
	{
		ClearPendingInteraction();
		PlayerController->StopMovement();
		PlayerController->RequestInteractWithActor(TargetActor);
		return;
	}

	// Out of range: set up walk-up movement target and poll until arrival.
	PendingInteractActor = TargetActor;
	PendingMovementTarget.Actor = TargetActor;
	PendingMovementTarget.Position = TargetActor->GetActorLocation();
	PendingMovementTarget.AcceptanceRadius = FMath::Max(60.0f, Def.InteractionRange * 0.8f);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			InteractArrivalTimerHandle,
			this,
			&UInteractionComponent::OnInteractArrivalTick,
			InteractArrivalPollInterval,
			true);
	}
}

void UInteractionComponent::OnInteractArrivalTick()
{
	AActor* TargetActor = PendingInteractActor.Get();
	if (!IsValid(TargetActor) || !UMHInteractionLibrary::IsInteractableActor(TargetActor))
	{
		ClearPendingInteraction();
		return;
	}

	AOnsetPlayerController* PlayerController = Cast<AOnsetPlayerController>(GetOwner());
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Pawn)
	{
		ClearPendingInteraction();
		return;
	}

	if (UMHInteractionLibrary::IsWithinInteractionRange(Pawn, TargetActor, 1.0f))
	{
		ClearPendingInteraction();
		PlayerController->StopMovement();
		PlayerController->RequestInteractWithActor(TargetActor);
	}
}

bool UInteractionComponent::TryInteractWithFocused()
{
	AOnsetPlayerController* PlayerController = Cast<AOnsetPlayerController>(GetOwner());
	if (!PlayerController)
	{
		return false;
	}

	if (!FocusedInteractableActor.IsValid())
	{
		FMHInteractionDefinition UnusedDef;
		bool bHasFocus = false;
		UpdateProximityFocus(UnusedDef, bHasFocus);
	}

	if (AActor* Target = FocusedInteractableActor.Get())
	{
		PlayerController->RequestInteractWithActor(Target);
		return true;
	}

	return false;
}

void UInteractionComponent::UpdateProximityFocus(FMHInteractionDefinition& OutFocusedDefinition, bool& bOutHasFocus)
{
	bOutHasFocus = false;
	FocusedInteractableActor.Reset();

	AOnsetPlayerController* PlayerController = Cast<AOnsetPlayerController>(GetOwner());
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	UWorld* World = GetWorld();
	if (!Pawn || !World)
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OnsetProximityInteractScan), false, Pawn);

	World->OverlapMultiByObjectType(
		Overlaps,
		Pawn->GetActorLocation(),
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(ProximitySearchRadius),
		QueryParams);

	float BestDistSq = TNumericLimits<float>::Max();
	AActor* BestActor = nullptr;
	FMHInteractionDefinition BestDef;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!IsValid(Candidate) || Candidate == Pawn)
		{
			continue;
		}

		if (!UMHInteractionLibrary::IsInteractableActor(Candidate))
		{
			continue;
		}

		FMHInteractionDefinition CandidateDef;
		if (!UMHInteractionLibrary::GetInteractionDefinition(Candidate, CandidateDef))
		{
			continue;
		}

		if (CandidateDef.TriggerMode == EMHInteractionTriggerMode::OverlapOnly)
		{
			continue;
		}

		FMHInteractionContext Context;
		Context.InstigatorActor = Pawn;
		Context.InstigatorController = PlayerController;
		Context.TargetActor = Candidate;
		Context.bTriggeredByOverlap = false;

		FText FailureReason;
		if (!UMHInteractionLibrary::ValidateInteraction(Context, FailureReason, 1.0f))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(Pawn->GetActorLocation(), Candidate->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestActor = Candidate;
			BestDef = CandidateDef;
		}
	}

	if (BestActor)
	{
		FocusedInteractableActor = BestActor;
		OutFocusedDefinition = BestDef;
		bOutHasFocus = true;
	}
}

void UInteractionComponent::ClearPendingInteraction()
{
	PendingInteractActor.Reset();
	PendingMovementTarget.Position = FVector::ZeroVector;
	PendingMovementTarget.Actor = nullptr;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InteractArrivalTimerHandle);
	}
}
