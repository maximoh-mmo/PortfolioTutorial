// Fill out your copyright notice in the Description page of Project Settings.

#include "NPC/OnsetNPC.h"

#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/OnsetDialogueLibrary.h"
#include "Data/OnsetQuestTypes.h"
#include "Data/OnsetRankTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "OnsetAppearanceComponent.h"
#include "OnsetAuthoredAppearance.h"
#include "Player/OnsetPlayerCharacter.h"
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

		if (UOnsetAuthoredAppearance* Authored = Def.AuthoredAppearance.LoadSynchronous())
		{
			UOnsetAppearanceComponent* AppComp = FindComponentByClass<UOnsetAppearanceComponent>();
			if (!AppComp)
			{
				AppComp = NewObject<UOnsetAppearanceComponent>(this, TEXT("NPCAppearance"));
				AppComp->RegisterComponent();
				AppComp->BindExternalBodyMesh(GetMesh());
			}
			AppComp->ApplyRecipe(Authored->Recipe);
			if (UOnsetAnimationProfile* Prof = Authored->AnimationProfile.LoadSynchronous())
			{
				if (!Prof->SaluteMontage.IsNull() && !ProtocolSaluteMontage)
				{
					ProtocolSaluteMontage = Prof->SaluteMontage.LoadSynchronous();
				}
			}
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

	// --- Confederate Military & Governance Protocol Check ---
	const FOnsetNPCDefinition Def = GetEffectiveNPCDefinition();
	if (AOnsetPlayerCharacter* PlayerChar = Cast<AOnsetPlayerCharacter>(Context.InstigatorActor))
	{
		const EOnsetRankId PlayerRank = PlayerChar->MilitaryRank;
		const EOnsetRankId NPCRank = Def.MilitaryRank;
		const EOnsetProtocolPrecedence Precedence = UOnsetRankLibrary::ComparePrecedence(PlayerRank, NPCRank);

		if (Precedence != EOnsetProtocolPrecedence::CivilianEtiquette)
		{
			const FText PlayerName = FText::FromString(TEXT("Operative"));
			const FText NPCName = !Def.DisplayName.IsEmpty() ? Def.DisplayName : FText::FromName(NPCRowName);

			const FText BannerText = UOnsetRankLibrary::BuildProtocolBannerText(PlayerName, PlayerRank, NPCName, NPCRank);
			const FText AckLine = (Precedence == EOnsetProtocolPrecedence::InitiatorIsSubordinate)
				? UOnsetRankLibrary::GetAcknowledgementLine(NPCRank, PlayerRank)
				: UOnsetRankLibrary::GetAcknowledgementLine(PlayerRank, NPCRank);

			if (PC)
			{
				PC->Client_ShowProtocolInteraction(BannerText, AckLine);
			}

			// Subordinate salutes / performs protocol first
			if (Precedence == EOnsetProtocolPrecedence::InitiatorIsSubordinate)
			{
				PlayerChar->Multicast_PlayProtocolGesture(Def.ProtocolStyle);
				Multicast_PlayProtocolGesture(Def.ProtocolStyle);
			}
			else if (Precedence == EOnsetProtocolPrecedence::InitiatorIsSuperior)
			{
				Multicast_PlayProtocolGesture(Def.ProtocolStyle);
				PlayerChar->Multicast_PlayProtocolGesture(Def.ProtocolStyle);
			}
			else
			{
				// Peers
				PlayerChar->Multicast_PlayProtocolGesture(Def.ProtocolStyle);
				Multicast_PlayProtocolGesture(Def.ProtocolStyle);
			}
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

void AOnsetNPC::Multicast_PlayProtocolGesture_Implementation(EOnsetProtocolStyle Style)
{
	if (ProtocolSaluteMontage && GetMesh() && GetMesh()->GetAnimInstance())
	{
		PlayAnimMontage(ProtocolSaluteMontage);
	}
	if (ProtocolSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ProtocolSound, GetActorLocation());
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
