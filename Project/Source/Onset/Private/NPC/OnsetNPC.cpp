// Fill out your copyright notice in the Description page of Project Settings.

#include "NPC/OnsetNPC.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/OnsetDialogueLibrary.h"
#include "Data/OnsetQuestTypes.h"
#include "Net/UnrealNetwork.h"
#include "Player/OnsetPlayerController.h"
#include "Quest/UOnsetQuestComponent.h"
#include "UMHInteractableComponent.h"

AOnsetNPC::AOnsetNPC()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->InitCapsuleSize(42.0f, 96.0f);
		Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	}

	InteractableComponent = CreateDefaultSubobject<UMHInteractableComponent>(TEXT("InteractableComponent"));
	InteractableComponent->Definition.InteractableId = NPCRowName;
	InteractableComponent->Definition.DisplayName = FText::FromString(TEXT("Elder Vane"));
	InteractableComponent->Definition.ActionPrompt = NSLOCTEXT("OnsetNPC", "TalkPrompt", "Talk");
	InteractableComponent->Definition.InteractionRange = 280.0f;
	InteractableComponent->Definition.TriggerMode = EMHInteractionTriggerMode::ExplicitOnly;

	InlineNPCDefinition.DisplayName = FText::FromString(TEXT("Villager"));
	InlineNPCDefinition.InteractionPrompt = NSLOCTEXT("OnsetNPC", "TalkPrompt", "Talk");
	InlineNPCDefinition.DefaultDialogueRow = FName(TEXT("ElderVane_Intro"));

	Tags.Add(FName(TEXT("NPC")));
}

void AOnsetNPC::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AOnsetNPC, NPCRowName);
	DOREPLIFETIME(AOnsetNPC, bUseInlineDefinition);
	DOREPLIFETIME(AOnsetNPC, InlineNPCDefinition);
}

void AOnsetNPC::BeginPlay()
{
	Super::BeginPlay();

	if (InteractableComponent)
	{
		InteractableComponent->OnInteractedNative.AddUObject(this, &AOnsetNPC::HandleInteracted);
		InteractableComponent->OnModifyDefinition.BindUObject(this, &AOnsetNPC::ModifyInteractionDefinition);

		const FOnsetNPCDefinition Def = GetEffectiveNPCDefinition();
		if (!NPCRowName.IsNone())
		{
			InteractableComponent->Definition.InteractableId = NPCRowName;
		}
		if (!Def.DisplayName.IsEmpty())
		{
			InteractableComponent->Definition.DisplayName = Def.DisplayName;
		}
		if (!Def.InteractionPrompt.IsEmpty())
		{
			InteractableComponent->Definition.ActionPrompt = Def.InteractionPrompt;
		}
	}
}

FOnsetNPCDefinition AOnsetNPC::GetEffectiveNPCDefinition() const
{
	if (!bUseInlineDefinition && !NPCRowName.IsNone())
	{
		if (const FOnsetNPCDefinition* TableDef = UOnsetDialogueLibrary::GetNPCDefinition(NPCRowName))
		{
			return *TableDef;
		}
	}
	return InlineNPCDefinition;
}

FName AOnsetNPC::ResolveStartingDialogueRow(const UOnsetQuestComponent* QuestComponent) const
{
	const FOnsetNPCDefinition Def = GetEffectiveNPCDefinition();
	return UOnsetDialogueLibrary::ResolveEntryDialogueRow(Def, QuestComponent);
}

void AOnsetNPC::ModifyInteractionDefinition(FMHInteractionDefinition& InOutDefinition)
{
	const FOnsetNPCDefinition Def = GetEffectiveNPCDefinition();
	if (!NPCRowName.IsNone() && InOutDefinition.InteractableId.IsNone())
	{
		InOutDefinition.InteractableId = NPCRowName;
	}
	if (!Def.DisplayName.IsEmpty())
	{
		InOutDefinition.DisplayName = Def.DisplayName;
	}
	if (!Def.InteractionPrompt.IsEmpty())
	{
		InOutDefinition.ActionPrompt = Def.InteractionPrompt;
	}
}

void AOnsetNPC::HandleInteracted(const FMHInteractionContext& Context)
{
	if (!HasAuthority() || !Context.InstigatorActor)
	{
		return;
	}

	if (bFaceInteractingPlayer)
	{
		FVector ToInstigator = Context.InstigatorActor->GetActorLocation() - GetActorLocation();
		ToInstigator.Z = 0.0f;
		if (!ToInstigator.IsNearlyZero())
		{
			SetActorRotation(FRotator(0.0f, ToInstigator.Rotation().Yaw, 0.0f));
		}
	}

	UOnsetQuestComponent* QuestComp = Context.InstigatorActor->FindComponentByClass<UOnsetQuestComponent>();
	const FName EffectiveId = !NPCRowName.IsNone() ? NPCRowName : InteractableComponent->Definition.InteractableId;
	if (QuestComp && !EffectiveId.IsNone())
	{
		QuestComp->ReportObjectiveProgress(EOnsetQuestObjectiveType::Interact, EffectiveId, 1);
	}

	BP_OnPlayerInteracted(Context.InstigatorActor);

	AOnsetPlayerController* PC = Cast<AOnsetPlayerController>(Context.InstigatorController);
	if (!PC)
	{
		if (APawn* InstigatorPawn = Cast<APawn>(Context.InstigatorActor))
		{
			PC = Cast<AOnsetPlayerController>(InstigatorPawn->GetController());
		}
	}

	if (PC)
	{
		const FName EntryRow = ResolveStartingDialogueRow(QuestComp);
		if (!EntryRow.IsNone())
		{
			PC->Client_OpenDialogue(this, EntryRow);
		}
	}
}

void AOnsetNPC::HandleDialogueChoiceSelected(AOnsetPlayerController* InteractingPC, FName CurrentDialogueRow, int32 ChoiceIndex)
{
	if (!HasAuthority() || !InteractingPC)
	{
		return;
	}

	APawn* PlayerPawn = InteractingPC->GetPawn();
	if (!PlayerPawn)
	{
		InteractingPC->Client_CloseDialogue();
		return;
	}

	const FOnsetDialogueRow* Row = UOnsetDialogueLibrary::GetDialogueRow(CurrentDialogueRow);
	if (!Row || !Row->Choices.IsValidIndex(ChoiceIndex))
	{
		InteractingPC->Client_CloseDialogue();
		return;
	}

	UOnsetQuestComponent* QuestComp = PlayerPawn->FindComponentByClass<UOnsetQuestComponent>();
	const FOnsetDialogueChoice& Choice = Row->Choices[ChoiceIndex];

	if (!UOnsetDialogueLibrary::IsChoiceAvailable(Choice, QuestComp))
	{
		InteractingPC->Client_CloseDialogue();
		return;
	}

	if (QuestComp)
	{
		if (!Choice.GrantQuestOnSelect.IsNone())
		{
			QuestComp->AcceptQuest(Choice.GrantQuestOnSelect);
		}
		if (!Choice.CompleteQuestOnSelect.IsNone())
		{
			QuestComp->CompleteQuest(Choice.CompleteQuestOnSelect);
		}
		if (!Choice.ReportGenericObjectiveTarget.IsNone())
		{
			QuestComp->ReportObjectiveProgress(EOnsetQuestObjectiveType::Generic, Choice.ReportGenericObjectiveTarget, 1);
		}
	}

	if (!Choice.NextDialogueRow.IsNone() && UOnsetDialogueLibrary::GetDialogueRow(Choice.NextDialogueRow) != nullptr)
	{
		InteractingPC->Client_OpenDialogue(this, Choice.NextDialogueRow);
	}
	else
	{
		InteractingPC->Client_CloseDialogue();
	}
}
