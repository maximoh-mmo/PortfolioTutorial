// Fill out your copyright notice in the Description page of Project Settings.

#include "World/OnsetDoor.h"
#include "UObject/ConstructorHelpers.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/OnsetQuestTypes.h"
#include "Inventory/UOnsetInventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Quest/UOnsetQuestComponent.h"
#include "UMHInteractableComponent.h"

AOnsetDoor::AOnsetDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	DoorPivot = CreateDefaultSubobject<USceneComponent>(TEXT("DoorPivot"));
	DoorPivot->SetupAttachment(RootScene);

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(DoorPivot);
	DoorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	DoorMesh->SetCollisionResponseToAllChannels(ECR_Block);
	DoorMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultDoorMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (DefaultDoorMesh.Succeeded())
	{
		DoorMesh->SetStaticMesh(DefaultDoorMesh.Object);
		DoorMesh->SetRelativeScale3D(FVector(0.3f, 2.0f, 2.5f));
	}

	InteractableComponent = CreateDefaultSubobject<UMHInteractableComponent>(TEXT("InteractableComponent"));
	InteractableComponent->Definition.InteractableId = FName(TEXT("Door"));
	InteractableComponent->Definition.DisplayName = NSLOCTEXT("OnsetDoor", "DefaultDoorName", "Door");
	InteractableComponent->Definition.ActionPrompt = NSLOCTEXT("OnsetDoor", "OpenPrompt", "Open");
	InteractableComponent->Definition.InteractionRange = 260.0f;
	InteractableComponent->Definition.TriggerMode = EMHInteractionTriggerMode::ExplicitOnly;
}

void AOnsetDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AOnsetDoor, bIsOpen);
	DOREPLIFETIME(AOnsetDoor, bIsLocked);
}

void AOnsetDoor::BeginPlay()
{
	Super::BeginPlay();

	if (DoorPivot)
	{
		InitialPivotLocation = DoorPivot->GetRelativeLocation();
		InitialPivotRotation = DoorPivot->GetRelativeRotation();
	}

	CurrentOpenAlpha = bIsOpen ? 1.0f : 0.0f;
	UpdateDoorTransform(CurrentOpenAlpha);

	if (InteractableComponent)
	{
		InteractableComponent->OnCanInteractQuery.BindUObject(this, &AOnsetDoor::QueryCanInteract);
		InteractableComponent->OnModifyDefinition.BindUObject(this, &AOnsetDoor::ModifyInteractionDefinition);
		InteractableComponent->OnInteractedNative.AddUObject(this, &AOnsetDoor::HandleInteracted);
	}
}

void AOnsetDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const float TargetAlpha = bIsOpen ? 1.0f : 0.0f;
	CurrentOpenAlpha = FMath::FInterpConstantTo(CurrentOpenAlpha, TargetAlpha, DeltaTime, OpenCloseSpeed);
	UpdateDoorTransform(CurrentOpenAlpha);

	if (FMath::IsNearlyEqual(CurrentOpenAlpha, TargetAlpha, KINDA_SMALL_NUMBER))
	{
		CurrentOpenAlpha = TargetAlpha;
		UpdateDoorTransform(CurrentOpenAlpha);
		SetActorTickEnabled(false);
	}
}

void AOnsetDoor::UpdateDoorTransform(float Alpha)
{
	if (!DoorPivot)
	{
		return;
	}

	const FVector NewLocation = FMath::Lerp(InitialPivotLocation, InitialPivotLocation + OpenLocationOffset, Alpha);
	const FQuat StartQuat = InitialPivotRotation.Quaternion();
	const FQuat EndQuat = (InitialPivotRotation + OpenRotationOffset).Quaternion();
	const FQuat InterpQuat = FQuat::Slerp(StartQuat, EndQuat, Alpha);

	DoorPivot->SetRelativeLocationAndRotation(NewLocation, InterpQuat.Rotator());
}

void AOnsetDoor::SetDoorOpen(bool bNewOpen)
{
	if (!HasAuthority())
	{
		return;
	}

	if (bIsOpen != bNewOpen)
	{
		bIsOpen = bNewOpen;
		OnRep_IsOpen();
	}
}

void AOnsetDoor::ToggleDoor(bool bForceUnlock)
{
	if (!HasAuthority())
	{
		return;
	}

	if (bForceUnlock && bIsLocked)
	{
		bIsLocked = false;
	}

	if (bIsLocked)
	{
		return;
	}

	SetDoorOpen(!bIsOpen);
}

void AOnsetDoor::UnlockDoor()
{
	if (HasAuthority())
	{
		bIsLocked = false;
	}
}

void AOnsetDoor::OnRep_IsOpen()
{
	SetActorTickEnabled(true);
	BP_OnDoorStateChanged(bIsOpen);
}

bool AOnsetDoor::CanPawnUnlockDoor(const AActor* InstigatorPawn, FText& OutFailureReason) const
{
	if (!bIsLocked)
	{
		return true;
	}

	if (!InstigatorPawn)
	{
		OutFailureReason = NSLOCTEXT("OnsetDoor", "Locked", "Locked.");
		return false;
	}

	bool bHasUnlockCondition = false;

	if (!RequiredQuestComplete.IsNone())
	{
		bHasUnlockCondition = true;
		const UOnsetQuestComponent* QuestComp = InstigatorPawn->FindComponentByClass<UOnsetQuestComponent>();
		if (!QuestComp || !QuestComp->IsQuestComplete(RequiredQuestComplete))
		{
			OutFailureReason = NSLOCTEXT("OnsetDoor", "LockedByQuest", "Sealed by an unfinished task.");
			return false;
		}
	}

	if (!RequiredKeyItemRow.IsNone())
	{
		bHasUnlockCondition = true;
		const UOnsetInventoryComponent* InvComp = InstigatorPawn->FindComponentByClass<UOnsetInventoryComponent>();
		bool bHasKey = false;
		if (InvComp)
		{
			for (const FOnsetInventoryEntry& Entry : InvComp->GetItems())
			{
				if (Entry.RowName == RequiredKeyItemRow && Entry.Count > 0)
				{
					bHasKey = true;
					break;
				}
			}
		}

		if (!bHasKey)
		{
			OutFailureReason = NSLOCTEXT("OnsetDoor", "MissingKey", "Requires a key.");
			return false;
		}
	}

	if (!bHasUnlockCondition)
	{
		// Locked by a mechanism/switch with no key or quest override
		OutFailureReason = NSLOCTEXT("OnsetDoor", "LockedByMechanism", "Locked from elsewhere.");
		return false;
	}

	return true;
}

bool AOnsetDoor::QueryCanInteract(const FMHInteractionContext& Context, FText& OutFailureReason) const
{
	if (bIsLocked)
	{
		return CanPawnUnlockDoor(Context.InstigatorActor, OutFailureReason);
	}
	return true;
}

void AOnsetDoor::ModifyInteractionDefinition(FMHInteractionDefinition& InOutDefinition)
{
	if (bIsLocked)
	{
		InOutDefinition.ActionPrompt = NSLOCTEXT("OnsetDoor", "UnlockPrompt", "Unlock");
	}
	else if (bIsOpen)
	{
		InOutDefinition.ActionPrompt = NSLOCTEXT("OnsetDoor", "ClosePrompt", "Close");
	}
	else
	{
		InOutDefinition.ActionPrompt = NSLOCTEXT("OnsetDoor", "OpenPrompt", "Open");
	}
}

void AOnsetDoor::HandleInteracted(const FMHInteractionContext& Context)
{
	if (!HasAuthority())
	{
		return;
	}

	if (bIsLocked)
	{
		FText FailureReason;
		if (!CanPawnUnlockDoor(Context.InstigatorActor, FailureReason))
		{
			return;
		}

		if (bConsumeKeyOnUnlock && !RequiredKeyItemRow.IsNone() && Context.InstigatorActor)
		{
			if (UOnsetInventoryComponent* InvComp = Context.InstigatorActor->FindComponentByClass<UOnsetInventoryComponent>())
			{
				for (const FOnsetInventoryEntry& Entry : InvComp->GetItems())
				{
					if (Entry.RowName == RequiredKeyItemRow && Entry.Count > 0)
					{
						InvComp->RemoveItem(Entry.Category, RequiredKeyItemRow, 1);
						break;
					}
				}
			}
		}

		bIsLocked = false;
	}

	SetDoorOpen(!bIsOpen);

	if (Context.InstigatorActor)
	{
		if (UOnsetQuestComponent* QuestComp = Context.InstigatorActor->FindComponentByClass<UOnsetQuestComponent>())
		{
			const FName DoorId = InteractableComponent ? InteractableComponent->Definition.InteractableId : NAME_None;
			if (!DoorId.IsNone())
			{
				QuestComp->ReportObjectiveProgress(EOnsetQuestObjectiveType::Interact, DoorId, 1);
			}
		}
	}
}
