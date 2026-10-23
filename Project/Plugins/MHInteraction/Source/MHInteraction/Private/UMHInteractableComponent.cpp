#include "UMHInteractableComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "MHInteractionModule.h"
#include "Net/UnrealNetwork.h"
#include "UMHInteractionLibrary.h"

UMHInteractableComponent::UMHInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UMHInteractableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMHInteractableComponent, Definition);
	DOREPLIFETIME(UMHInteractableComponent, bEnabled);
	DOREPLIFETIME(UMHInteractableComponent, bHasBeenUsed);
}

void UMHInteractableComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Definition.DisplayName.IsEmpty() && GetOwner())
	{
		Definition.DisplayName = FText::FromString(GetOwner()->GetName());
	}
}

void UMHInteractableComponent::SetEnabled(bool bInEnabled)
{
	bEnabled = bInEnabled;
}

void UMHInteractableComponent::ResetUsage()
{
	bHasBeenUsed = false;
	LastInteractionWorldTime = -1000.0;
}

void UMHInteractableComponent::BindOverlapPrimitive(UPrimitiveComponent* Primitive)
{
	if (BoundOverlapPrimitive)
	{
		BoundOverlapPrimitive->OnComponentBeginOverlap.RemoveDynamic(this, &UMHInteractableComponent::HandlePrimitiveBeginOverlap);
	}

	BoundOverlapPrimitive = Primitive;

	if (BoundOverlapPrimitive)
	{
		BoundOverlapPrimitive->OnComponentBeginOverlap.AddUniqueDynamic(this, &UMHInteractableComponent::HandlePrimitiveBeginOverlap);
	}
}

void UMHInteractableComponent::GetInteractionDefinition_Implementation(FMHInteractionDefinition& OutDefinition) const
{
	OutDefinition = Definition;
	if (OnModifyDefinition.IsBound())
	{
		OnModifyDefinition.Execute(OutDefinition);
	}
}

bool UMHInteractableComponent::CanInteract_Implementation(const FMHInteractionContext& Context, FText& OutFailureReason) const
{
	if (!bEnabled)
	{
		OutFailureReason = NSLOCTEXT("MHInteraction", "Disabled", "Cannot interact right now.");
		return false;
	}

	if (bSingleUse && bHasBeenUsed)
	{
		OutFailureReason = NSLOCTEXT("MHInteraction", "AlreadyUsed", "Already used.");
		return false;
	}

	if (Context.bTriggeredByOverlap)
	{
		if (Definition.TriggerMode == EMHInteractionTriggerMode::ExplicitOnly)
		{
			OutFailureReason = NSLOCTEXT("MHInteraction", "ExplicitOnly", "Requires explicit interaction.");
			return false;
		}
	}
	else
	{
		if (Definition.TriggerMode == EMHInteractionTriggerMode::OverlapOnly)
		{
			OutFailureReason = NSLOCTEXT("MHInteraction", "OverlapOnly", "Triggered by proximity only.");
			return false;
		}
	}

	if (CooldownSeconds > 0.0f && GetWorld())
	{
		const double Now = GetWorld()->GetTimeSeconds();
		if ((Now - LastInteractionWorldTime) < static_cast<double>(CooldownSeconds))
		{
			OutFailureReason = NSLOCTEXT("MHInteraction", "OnCooldown", "Not ready yet.");
			return false;
		}
	}

	if (OnCanInteractQuery.IsBound())
	{
		if (!OnCanInteractQuery.Execute(Context, OutFailureReason))
		{
			return false;
		}
	}

	return true;
}

bool UMHInteractableComponent::ExecuteInteraction_Implementation(const FMHInteractionContext& Context)
{
	FText FailureReason;
	if (!IMHInteractable::Execute_CanInteract(this, Context, FailureReason))
	{
		UE_LOG(LogMHInteraction, Verbose, TEXT("ExecuteInteraction rejected on %s: %s"),
			*GetNameSafe(GetOwner()), *FailureReason.ToString());
		return false;
	}

	if (GetWorld())
	{
		LastInteractionWorldTime = GetWorld()->GetTimeSeconds();
	}

	if (bSingleUse)
	{
		bHasBeenUsed = true;
	}

	OnInteractedNative.Broadcast(Context);
	OnInteracted.Broadcast(Context);

	return true;
}

void UMHInteractableComponent::HandlePrimitiveBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !OtherActor || OtherActor == GetOwner())
	{
		return;
	}

	if (Definition.TriggerMode == EMHInteractionTriggerMode::ExplicitOnly)
	{
		return;
	}

	APawn* InstigatorPawn = Cast<APawn>(OtherActor);
	if (!InstigatorPawn || !InstigatorPawn->IsPlayerControlled())
	{
		return;
	}

	FMHInteractionContext Context;
	Context.InstigatorActor = InstigatorPawn;
	Context.InstigatorController = InstigatorPawn->GetController();
	Context.TargetActor = GetOwner();
	Context.bTriggeredByOverlap = true;

	FText ErrorText;
	if (UMHInteractionLibrary::ValidateInteraction(Context, ErrorText))
	{
		IMHInteractable::Execute_ExecuteInteraction(this, Context);
	}
}
