#include "UMHInteractionLibrary.h"

#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "IMHInteractable.h"
#include "MHInteractionModule.h"

UObject* UMHInteractionLibrary::ResolveInteractableObject(AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		return nullptr;
	}

	if (TargetActor->GetClass()->ImplementsInterface(UMHInteractable::StaticClass()))
	{
		return TargetActor;
	}

	return TargetActor->FindComponentByInterface(UMHInteractable::StaticClass());
}

bool UMHInteractionLibrary::IsInteractableActor(AActor* TargetActor)
{
	return ResolveInteractableObject(TargetActor) != nullptr;
}

bool UMHInteractionLibrary::GetInteractionDefinition(AActor* TargetActor, FMHInteractionDefinition& OutDefinition)
{
	UObject* InteractableObj = ResolveInteractableObject(TargetActor);
	if (!InteractableObj)
	{
		return false;
	}

	IMHInteractable::Execute_GetInteractionDefinition(InteractableObj, OutDefinition);
	return true;
}

bool UMHInteractionLibrary::IsWithinInteractionRange(const AActor* InstigatorActor, AActor* TargetActor, float RangeToleranceScale)
{
	if (!IsValid(InstigatorActor) || !IsValid(TargetActor))
	{
		return false;
	}

	FMHInteractionDefinition Def;
	if (!GetInteractionDefinition(TargetActor, Def))
	{
		return false;
	}

	const float MaxRange = FMath::Max(25.0f, Def.InteractionRange) * FMath::Max(0.1f, RangeToleranceScale);
	const float Dist2D = FVector::Dist2D(InstigatorActor->GetActorLocation(), TargetActor->GetActorLocation());
	const float VerticalDelta = FMath::Abs(InstigatorActor->GetActorLocation().Z - TargetActor->GetActorLocation().Z);

	return (Dist2D <= MaxRange) && (VerticalDelta <= FMath::Max(MaxRange, 250.0f));
}

bool UMHInteractionLibrary::ValidateInteraction(const FMHInteractionContext& Context, FText& OutFailureReason, float RangeToleranceScale)
{
	if (!IsValid(Context.InstigatorActor) || !IsValid(Context.TargetActor))
	{
		OutFailureReason = NSLOCTEXT("MHInteraction", "InvalidActors", "Invalid interaction target.");
		return false;
	}

	UObject* InteractableObj = ResolveInteractableObject(Context.TargetActor);
	if (!InteractableObj)
	{
		OutFailureReason = NSLOCTEXT("MHInteraction", "NotInteractable", "Target is not interactable.");
		return false;
	}

	FMHInteractionDefinition Def;
	IMHInteractable::Execute_GetInteractionDefinition(InteractableObj, Def);

	if (!Context.bTriggeredByOverlap && !IsWithinInteractionRange(Context.InstigatorActor, Context.TargetActor, RangeToleranceScale))
	{
		OutFailureReason = NSLOCTEXT("MHInteraction", "OutOfRange", "Target is out of range.");
		return false;
	}

	if (Def.bRequiresLineOfSight)
	{
		if (UWorld* World = Context.InstigatorActor->GetWorld())
		{
			FCollisionQueryParams Params(SCENE_QUERY_STAT(MHInteractionLOS), false);
			Params.AddIgnoredActor(Context.InstigatorActor);
			Params.AddIgnoredActor(Context.TargetActor);

			const FVector Start = Context.InstigatorActor->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
			const FVector End = Context.TargetActor->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
			FHitResult Hit;
			if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
			{
				OutFailureReason = NSLOCTEXT("MHInteraction", "BlockedLOS", "Target is not in line of sight.");
				return false;
			}
		}
	}

	return IMHInteractable::Execute_CanInteract(InteractableObj, Context, OutFailureReason);
}

bool UMHInteractionLibrary::TryExecuteInteraction(const FMHInteractionContext& Context, FText& OutFailureReason)
{
	if (!ValidateInteraction(Context, OutFailureReason))
	{
		return false;
	}

	UObject* InteractableObj = ResolveInteractableObject(Context.TargetActor);
	if (!InteractableObj)
	{
		return false;
	}

	return IMHInteractable::Execute_ExecuteInteraction(InteractableObj, Context);
}
