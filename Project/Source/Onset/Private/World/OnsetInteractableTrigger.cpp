// Fill out your copyright notice in the Description page of Project Settings.

#include "World/OnsetInteractableTrigger.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/OnsetQuestTypes.h"
#include "IMHInteractable.h"
#include "Net/UnrealNetwork.h"
#include "Quest/UOnsetQuestComponent.h"
#include "UMHInteractableComponent.h"
#include "UMHInteractionLibrary.h"
#include "World/OnsetDoor.h"

AOnsetInteractableTrigger::AOnsetInteractableTrigger()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	TriggerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TriggerMesh"));
	TriggerMesh->SetupAttachment(RootScene);
	TriggerMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	TriggerMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	OverlapVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapVolume"));
	OverlapVolume->SetupAttachment(RootScene);
	OverlapVolume->InitBoxExtent(FVector(120.0f, 120.0f, 100.0f));
	OverlapVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverlapVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	InteractableComponent = CreateDefaultSubobject<UMHInteractableComponent>(TEXT("InteractableComponent"));
	InteractableComponent->Definition.InteractableId = FName(TEXT("WorldSwitch"));
	InteractableComponent->Definition.DisplayName = NSLOCTEXT("OnsetTrigger", "DefaultSwitchName", "Mechanism");
	InteractableComponent->Definition.ActionPrompt = NSLOCTEXT("OnsetTrigger", "ActivatePrompt", "Activate");
	InteractableComponent->Definition.InteractionRange = 250.0f;
	InteractableComponent->Definition.TriggerMode = EMHInteractionTriggerMode::ExplicitOnly;
}

void AOnsetInteractableTrigger::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AOnsetInteractableTrigger, bIsActivated);
}

void AOnsetInteractableTrigger::BeginPlay()
{
	Super::BeginPlay();

	if (InteractableComponent)
	{
		InteractableComponent->BindOverlapPrimitive(OverlapVolume);
		InteractableComponent->OnInteractedNative.AddUObject(this, &AOnsetInteractableTrigger::HandleInteracted);
	}
}

void AOnsetInteractableTrigger::OnRep_IsActivated()
{
	BP_OnTriggerStateChanged(bIsActivated);
}

void AOnsetInteractableTrigger::HandleInteracted(const FMHInteractionContext& Context)
{
	if (!HasAuthority())
	{
		return;
	}

	bIsActivated = !bIsActivated;
	OnRep_IsActivated();

	for (AActor* LinkedActor : LinkedActors)
	{
		if (!IsValid(LinkedActor) || LinkedActor == this)
		{
			continue;
		}

		if (AOnsetDoor* Door = Cast<AOnsetDoor>(LinkedActor))
		{
			Door->ToggleDoor(bUnlockLinkedDoors);
		}
		else if (UObject* LinkedInteractable = UMHInteractionLibrary::ResolveInteractableObject(LinkedActor))
		{
			FMHInteractionContext ForwardContext = Context;
			ForwardContext.TargetActor = LinkedActor;
			IMHInteractable::Execute_ExecuteInteraction(LinkedInteractable, ForwardContext);
		}
	}

	if (Context.InstigatorActor)
	{
		if (UOnsetQuestComponent* QuestComp = Context.InstigatorActor->FindComponentByClass<UOnsetQuestComponent>())
		{
			const FName TriggerId = InteractableComponent ? InteractableComponent->Definition.InteractableId : NAME_None;
			if (!TriggerId.IsNone())
			{
				QuestComp->ReportObjectiveProgress(EOnsetQuestObjectiveType::Interact, TriggerId, 1);
			}
			if (bReportReachLocation)
			{
				QuestComp->ReportReachLocation(GetActorLocation());
			}
		}
	}

	BP_OnTriggeredByPawn(Context.InstigatorActor, Context.bTriggeredByOverlap);
}
