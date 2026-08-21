// Fill out your copyright notice in the Description page of Project Settings.

#include "Quest/UOnsetQuestComponent.h"

#include "Core/OnsetBaseCharacter.h"
#include "Data/OnsetQuestLibrary.h"
#include "Dom/JsonObject.h"
#include "Inventory/UOnsetInventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/OnsetPlayerCharacter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

UOnsetQuestComponent::UOnsetQuestComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	// Replicate Quests to the owning client whenever registered on a replicated pawn.
	SetIsReplicatedByDefault(true);
}

void UOnsetQuestComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = false;
	Params.Condition = COND_OwnerOnly;
	DOREPLIFETIME_WITH_PARAMS_FAST(UOnsetQuestComponent, Quests, Params);
}

void UOnsetQuestComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	// Collect objectives are driven by the shared inventory's item-add signal.
	if (AOnsetBaseCharacter* OwnerCharacter = Cast<AOnsetBaseCharacter>(GetOwner()))
	{
		if (OwnerCharacter->InventoryComponent)
		{
			OwnerCharacter->InventoryComponent->OnItemAdded.AddUObject(this, &UOnsetQuestComponent::HandleItemAdded);
		}
	}
}

void UOnsetQuestComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AOnsetBaseCharacter* OwnerCharacter = Cast<AOnsetBaseCharacter>(GetOwner()))
	{
		if (OwnerCharacter->InventoryComponent)
		{
			OwnerCharacter->InventoryComponent->OnItemAdded.RemoveAll(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UOnsetQuestComponent::AcceptQuest(FName QuestRowName)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || QuestRowName.IsNone())
	{
		return;
	}
	if (HasQuest(QuestRowName))
	{
		return;
	}

	const FOnsetQuestDefinition* Def = UOnsetQuestLibrary::GetQuest(QuestRowName);
	if (!Def)
	{
		UE_LOG(LogTemp, Warning, TEXT("AcceptQuest: no DT_Quests row '%s'"), *QuestRowName.ToString());
		return;
	}

	FOnsetQuestState Quest;
	Quest.QuestRowName = QuestRowName;
	Quest.StageIndex = 0;
	for (const FOnsetQuestStageDefinition& StageDef : Def->Stages)
	{
		FOnsetQuestStageState StageState;
		for (const FOnsetQuestObjectiveDefinition& ObjDef : StageDef.Objectives)
		{
			FOnsetQuestObjectiveState ObjState;
			ObjState.Progress = 0;
			ObjState.bComplete = false;
			StageState.Objectives.Add(ObjState);
		}
		Quest.Stages.Add(StageState);
	}

	Quests.Add(Quest);

	// A quest with zero stages completes immediately; otherwise run completion logic.
	RecomputeCompletion();

	UE_LOG(LogTemp, Log, TEXT("Quest: accepted '%s' (%d stage(s))"), *QuestRowName.ToString(), Def->Stages.Num());
	OnQuestsChanged.Broadcast();
}

void UOnsetQuestComponent::CompleteQuest(FName QuestRowName)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	FOnsetQuestState* Quest = Quests.FindByPredicate(
		[QuestRowName](const FOnsetQuestState& Candidate) { return Candidate.QuestRowName == QuestRowName; });
	if (!Quest || Quest->bComplete)
	{
		return;
	}

	const FOnsetQuestDefinition* Def = UOnsetQuestLibrary::GetQuest(QuestRowName);
	if (!Def)
	{
		return;
	}

	Quest->bComplete = true;
	for (FOnsetQuestStageState& Stage : Quest->Stages)
	{
		Stage.bComplete = true;
		for (FOnsetQuestObjectiveState& Obj : Stage.Objectives)
		{
			Obj.bComplete = true;
		}
	}
	GrantRewards(*Def);

	UE_LOG(LogTemp, Log, TEXT("Quest: force-completed '%s'"), *QuestRowName.ToString());
	OnQuestsChanged.Broadcast();
}

bool UOnsetQuestComponent::HasQuest(FName QuestRowName) const
{
	return Quests.ContainsByPredicate(
		[QuestRowName](const FOnsetQuestState& Candidate) { return Candidate.QuestRowName == QuestRowName; });
}

void UOnsetQuestComponent::ReportObjectiveProgress(EOnsetQuestObjectiveType Type, FName Target, int32 Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Amount <= 0)
	{
		return;
	}

	const bool bChanged = ApplyProgress(Type, Target, Amount);
	if (bChanged)
	{
		RecomputeCompletion();
		OnQuestsChanged.Broadcast();
	}
}

void UOnsetQuestComponent::ReportReachLocation(FVector InLocation)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	bool bChanged = false;
	for (FOnsetQuestState& Quest : Quests)
	{
		if (Quest.bComplete || Quest.StageIndex >= Quest.Stages.Num())
		{
			continue;
		}
		const FOnsetQuestDefinition* Def = UOnsetQuestLibrary::GetQuest(Quest.QuestRowName);
		if (!Def || Quest.StageIndex >= Def->Stages.Num())
		{
			continue;
		}

		const FOnsetQuestStageDefinition& StageDef = Def->Stages[Quest.StageIndex];
		FOnsetQuestStageState& StageState = Quest.Stages[Quest.StageIndex];
		for (int32 i = 0; i < StageDef.Objectives.Num() && i < StageState.Objectives.Num(); ++i)
		{
			const FOnsetQuestObjectiveDefinition& ObjDef = StageDef.Objectives[i];
			FOnsetQuestObjectiveState& ObjState = StageState.Objectives[i];
			if (ObjState.bComplete || ObjDef.Type != EOnsetQuestObjectiveType::ReachLocation)
			{
				continue;
			}
			if (FVector::Dist(InLocation, ObjDef.Location) <= ObjDef.Radius)
			{
				ObjState.Progress = FMath::Max(1, ObjDef.Count);
				ObjState.bComplete = true;
				bChanged = true;
			}
		}
	}

	if (bChanged)
	{
		RecomputeCompletion();
		OnQuestsChanged.Broadcast();
	}
}

bool UOnsetQuestComponent::ApplyProgress(EOnsetQuestObjectiveType Type, FName Target, int32 Amount)
{
	bool bChanged = false;
	for (FOnsetQuestState& Quest : Quests)
	{
		if (Quest.bComplete || Quest.StageIndex >= Quest.Stages.Num())
		{
			continue;
		}
		const FOnsetQuestDefinition* Def = UOnsetQuestLibrary::GetQuest(Quest.QuestRowName);
		if (!Def || Quest.StageIndex >= Def->Stages.Num())
		{
			continue;
		}

		const FOnsetQuestStageDefinition& StageDef = Def->Stages[Quest.StageIndex];
		FOnsetQuestStageState& StageState = Quest.Stages[Quest.StageIndex];
		for (int32 i = 0; i < StageDef.Objectives.Num() && i < StageState.Objectives.Num(); ++i)
		{
			const FOnsetQuestObjectiveDefinition& ObjDef = StageDef.Objectives[i];
			FOnsetQuestObjectiveState& ObjState = StageState.Objectives[i];
			if (ObjState.bComplete || ObjDef.Type != Type)
			{
				continue;
			}
			// Empty target matches any target of the given type.
			if (!ObjDef.Target.IsNone() && ObjDef.Target != Target)
			{
				continue;
			}

			const int32 Required = FMath::Max(1, ObjDef.Count);
			ObjState.Progress = FMath::Min(Required, ObjState.Progress + Amount);
			if (ObjState.Progress >= Required)
			{
				ObjState.bComplete = true;
			}
			bChanged = true;
		}
	}
	return bChanged;
}

void UOnsetQuestComponent::RecomputeCompletion(bool bGrantRewards)
{
	for (FOnsetQuestState& Quest : Quests)
	{
		if (Quest.bComplete)
		{
			continue;
		}
		const FOnsetQuestDefinition* Def = UOnsetQuestLibrary::GetQuest(Quest.QuestRowName);
		if (!Def)
		{
			continue;
		}

		// Advance through stages until the active stage has an incomplete objective.
		bool bAdvanced = true;
		while (bAdvanced && !Quest.bComplete)
		{
			bAdvanced = false;

			if (Quest.StageIndex >= Def->Stages.Num() || Quest.StageIndex >= Quest.Stages.Num())
			{
				// Ran off the end (or zero-stage quest): done.
				Quest.bComplete = true;
				if (bGrantRewards)
				{
					GrantRewards(*Def);
				}
				break;
			}

			const FOnsetQuestStageDefinition& StageDef = Def->Stages[Quest.StageIndex];
			FOnsetQuestStageState& StageState = Quest.Stages[Quest.StageIndex];

			bool bStageComplete = true;
			for (int32 i = 0; i < StageDef.Objectives.Num(); ++i)
			{
				const FOnsetQuestObjectiveDefinition& ObjDef = StageDef.Objectives[i];
				if (i >= StageState.Objectives.Num())
				{
					bStageComplete = false;
					break;
				}
				// Re-derive from definition count so persisted progress always self-corrects.
				if (StageState.Objectives[i].Progress < FMath::Max(1, ObjDef.Count))
				{
					bStageComplete = false;
					break;
				}
			}

			if (!bStageComplete)
			{
				break;
			}

			StageState.bComplete = true;
			Quest.StageIndex++;
			bAdvanced = true;
		}
	}
}

void UOnsetQuestComponent::GrantRewards(const FOnsetQuestDefinition& Def)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (Def.XpReward > 0)
	{
		if (AOnsetPlayerCharacter* PlayerCharacter = Cast<AOnsetPlayerCharacter>(GetOwner()))
		{
			PlayerCharacter->GrantQuestXP(Def.XpReward);
		}
	}

	if (Def.RewardItems.Num() > 0)
	{
		if (AOnsetBaseCharacter* OwnerCharacter = Cast<AOnsetBaseCharacter>(GetOwner()))
		{
			if (OwnerCharacter->InventoryComponent)
			{
				TArray<FOnsetInventoryEntry> Entries;
				Entries.Reserve(Def.RewardItems.Num());
				for (const FOnsetQuestRewardItem& Reward : Def.RewardItems)
				{
					FOnsetInventoryEntry Entry;
					Entry.Category = Reward.Category;
					Entry.RowName = Reward.RowName;
					Entry.Count = FMath::Max(1, Reward.Count);
					Entries.Add(Entry);
				}
				OwnerCharacter->InventoryComponent->AddItems(Entries);
			}
		}
	}
}

void UOnsetQuestComponent::HandleItemAdded(EOnsetItemCategory Category, FName RowName, int32 Count)
{
	ReportObjectiveProgress(EOnsetQuestObjectiveType::Collect, RowName, Count);
}

void UOnsetQuestComponent::OnRep_Quests()
{
	OnQuestsChanged.Broadcast();
}

// --- Persistence ---

FString UOnsetQuestComponent::SerializeQuestsJSON() const
{
	TArray<TSharedPtr<FJsonValue>> QuestArray;
	for (const FOnsetQuestState& Quest : Quests)
	{
		TSharedPtr<FJsonObject> QuestObj = MakeShareable(new FJsonObject);
		QuestObj->SetStringField(TEXT("Row"), Quest.QuestRowName.ToString());
		QuestObj->SetNumberField(TEXT("Stage"), Quest.StageIndex);
		QuestObj->SetBoolField(TEXT("Done"), Quest.bComplete);

		TArray<TSharedPtr<FJsonValue>> StageArray;
		for (const FOnsetQuestStageState& Stage : Quest.Stages)
		{
			TArray<TSharedPtr<FJsonValue>> ObjArray;
			for (const FOnsetQuestObjectiveState& Obj : Stage.Objectives)
			{
				ObjArray.Add(MakeShareable(new FJsonValueNumber(Obj.Progress)));
			}
			StageArray.Add(MakeShareable(new FJsonValueArray(ObjArray)));
		}
		QuestObj->SetArrayField(TEXT("Stages"), StageArray);
		QuestArray.Add(MakeShareable(new FJsonValueObject(QuestObj)));
	}

	TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject);
	Root->SetArrayField(TEXT("Quests"), QuestArray);

	FString Out;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
	return Out;
}

void UOnsetQuestComponent::DeserializeQuestsJSON(const FString& JSON)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	Quests.Reset();
	if (JSON.IsEmpty() || JSON == TEXT("{}"))
	{
		return;
	}

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JSON);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return;
	}

	const TArray<TSharedPtr<FJsonValue>>* QuestArray = nullptr;
	if (!Root->TryGetArrayField(TEXT("Quests"), QuestArray))
	{
		return;
	}

	for (const TSharedPtr<FJsonValue>& QuestValue : *QuestArray)
	{
		const TSharedPtr<FJsonObject> QuestObj = QuestValue->AsObject();
		if (!QuestObj.IsValid())
		{
			continue;
		}

		const FName QuestRowName = FName(*QuestObj->GetStringField(TEXT("Row")));
		const FOnsetQuestDefinition* Def = UOnsetQuestLibrary::GetQuest(QuestRowName);
		if (!Def)
		{
			UE_LOG(LogTemp, Warning, TEXT("DeserializeQuestsJSON: skipping unknown quest '%s'"), *QuestRowName.ToString());
			continue;
		}

		FOnsetQuestState Quest;
		Quest.QuestRowName = QuestRowName;
		Quest.StageIndex = FMath::Max(0, QuestObj->GetIntegerField(TEXT("Stage")));
		QuestObj->TryGetBoolField(TEXT("Done"), Quest.bComplete);

		const TArray<TSharedPtr<FJsonValue>>* StageArray = nullptr;
		QuestObj->TryGetArrayField(TEXT("Stages"), StageArray);

		for (int32 si = 0; si < Def->Stages.Num(); ++si)
		{
			const FOnsetQuestStageDefinition& StageDef = Def->Stages[si];
			FOnsetQuestStageState StageState;

			const TArray<TSharedPtr<FJsonValue>>* ObjArray = nullptr;
			if (StageArray && StageArray->IsValidIndex(si))
			{
				ObjArray = &(*StageArray)[si]->AsArray();
			}

			for (int32 oi = 0; oi < StageDef.Objectives.Num(); ++oi)
			{
				FOnsetQuestObjectiveState ObjState;
				if (ObjArray && ObjArray->IsValidIndex(oi))
				{
					ObjState.Progress = FMath::Clamp(static_cast<int32>((*ObjArray)[oi]->AsNumber()), 0, FMath::Max(1, StageDef.Objectives[oi].Count));
				}
				ObjState.bComplete = ObjState.Progress >= FMath::Max(1, StageDef.Objectives[oi].Count);
				StageState.Objectives.Add(ObjState);
			}
			Quest.Stages.Add(StageState);
		}

		Quests.Add(Quest);
	}

	// Re-derive stage/quest completion + catch any auto-advance from persisted progress.
	// Do NOT re-grant rewards: they were already granted when the quest completed.
	RecomputeCompletion(false);
	OnQuestsChanged.Broadcast();
}