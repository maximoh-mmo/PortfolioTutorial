// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/OnsetDialogueLibrary.h"

#include "Engine/DataTable.h"
#include "Misc/ConfigCacheIni.h"
#include "Quest/UOnsetQuestComponent.h"

namespace OnsetDialogueLibraryInternal
{
	TStrongObjectPtr<UDataTable> CachedDialogueTable;
	TStrongObjectPtr<UDataTable> CachedNPCTable;
	TMap<FName, FOnsetDialogueRow> FallbackDialogueRows;
	TMap<FName, FOnsetNPCDefinition> FallbackNPCDefinitions;
	bool bDefaultsInitialized = false;

	void EnsureDefaultFallbacks()
	{
		if (bDefaultsInitialized)
		{
			return;
		}
		bDefaultsInitialized = true;

		// Seed a default starter NPC and branching dialogue chain ("DefaultGuide") so any placed AOnsetNPC
		// works out-of-the-box and demonstrates branching + quest offer/progress/completion.
		{
			FOnsetDialogueRow Intro;
			Intro.SpeakerName = FText::FromString(TEXT("Elder Vane"));
			Intro.DialogueText = FText::FromString(TEXT("Traveler, the corruption beyond the gate grows restless. Will you lend your blade to the vanguard?"));

			FOnsetDialogueChoice AcceptChoice;
			AcceptChoice.ResponseText = FText::FromString(TEXT("I'll clear the road. [Accept Quest: FirstBlood]"));
			AcceptChoice.RequiredQuestNotStarted = FName(TEXT("FirstBlood"));
			AcceptChoice.GrantQuestOnSelect = FName(TEXT("FirstBlood"));
			AcceptChoice.NextDialogueRow = FName(TEXT("ElderVane_Accepted"));
			Intro.Choices.Add(AcceptChoice);

			FOnsetDialogueChoice AskMore;
			AskMore.ResponseText = FText::FromString(TEXT("What lies beyond the gate?"));
			AskMore.NextDialogueRow = FName(TEXT("ElderVane_Lore"));
			Intro.Choices.Add(AskMore);

			FOnsetDialogueChoice LeaveChoice;
			LeaveChoice.ResponseText = FText::FromString(TEXT("Farewell."));
			LeaveChoice.NextDialogueRow = NAME_None;
			Intro.Choices.Add(LeaveChoice);

			FallbackDialogueRows.Add(FName(TEXT("ElderVane_Intro")), Intro);
		}

		{
			FOnsetDialogueRow Lore;
			Lore.SpeakerName = FText::FromString(TEXT("Elder Vane"));
			Lore.DialogueText = FText::FromString(TEXT("Ancient mechanisms seal the inner sanctum. Look for runic levers and warded doors along the ridge."));

			FOnsetDialogueChoice BackChoice;
			BackChoice.ResponseText = FText::FromString(TEXT("Understood. Let us speak of the task at hand."));
			BackChoice.NextDialogueRow = FName(TEXT("ElderVane_Intro"));
			Lore.Choices.Add(BackChoice);

			FOnsetDialogueChoice CloseChoice;
			CloseChoice.ResponseText = FText::FromString(TEXT("I'll keep an eye out. Farewell."));
			CloseChoice.NextDialogueRow = NAME_None;
			Lore.Choices.Add(CloseChoice);

			FallbackDialogueRows.Add(FName(TEXT("ElderVane_Lore")), Lore);
		}

		{
			FOnsetDialogueRow Accepted;
			Accepted.SpeakerName = FText::FromString(TEXT("Elder Vane"));
			Accepted.DialogueText = FText::FromString(TEXT("May your strikes be swift. Return once the threat is culled."));

			FOnsetDialogueChoice CloseChoice;
			CloseChoice.ResponseText = FText::FromString(TEXT("I'm on my way."));
			CloseChoice.NextDialogueRow = NAME_None;
			Accepted.Choices.Add(CloseChoice);

			FallbackDialogueRows.Add(FName(TEXT("ElderVane_Accepted")), Accepted);
		}

		{
			FOnsetDialogueRow Completed;
			Completed.SpeakerName = FText::FromString(TEXT("Elder Vane"));
			Completed.DialogueText = FText::FromString(TEXT("Well fought, champion. The outpost stands safer because of you."));

			FOnsetDialogueChoice CloseChoice;
			CloseChoice.ResponseText = FText::FromString(TEXT("Glad to help."));
			CloseChoice.NextDialogueRow = NAME_None;
			Completed.Choices.Add(CloseChoice);

			FallbackDialogueRows.Add(FName(TEXT("ElderVane_Completed")), Completed);
		}

		{
			FOnsetNPCDefinition ElderDef;
			ElderDef.DisplayName = FText::FromString(TEXT("Elder Vane"));
			ElderDef.TitleOrRole = FText::FromString(TEXT("Outpost Warden"));
			ElderDef.InteractionPrompt = FText::FromString(TEXT("Talk"));
			ElderDef.DefaultDialogueRow = FName(TEXT("ElderVane_Intro"));

			FOnsetNPCQuestDialogueOverride QuestOverride;
			QuestOverride.QuestRowName = FName(TEXT("FirstBlood"));
			QuestOverride.NotStartedDialogueRow = FName(TEXT("ElderVane_Intro"));
			QuestOverride.InProgressDialogueRow = FName(TEXT("ElderVane_Accepted"));
			QuestOverride.CompletedDialogueRow = FName(TEXT("ElderVane_Completed"));
			ElderDef.QuestDialogueOverrides.Add(QuestOverride);

			FallbackNPCDefinitions.Add(FName(TEXT("ElderVane")), ElderDef);
		}
	}
}

FString UOnsetDialogueLibrary::GetDialogueTablePath()
{
	FString Path;
	GConfig->GetString(TEXT("Onset.Gameplay"), TEXT("DialogueDataTable"), Path, GEngineIni);
	if (Path.IsEmpty())
	{
		Path = TEXT("/Game/Data/DT_Dialogues.DT_Dialogues");
	}
	return Path;
}

FString UOnsetDialogueLibrary::GetNPCTablePath()
{
	FString Path;
	GConfig->GetString(TEXT("Onset.Gameplay"), TEXT("NPCDataTable"), Path, GEngineIni);
	if (Path.IsEmpty())
	{
		Path = TEXT("/Game/Data/DT_NPCs.DT_NPCs");
	}
	return Path;
}

UDataTable* UOnsetDialogueLibrary::GetDialogueTable()
{
	if (OnsetDialogueLibraryInternal::CachedDialogueTable)
	{
		return OnsetDialogueLibraryInternal::CachedDialogueTable.Get();
	}

	const FString TablePath = GetDialogueTablePath();
	if (FPackageName::DoesPackageExist(TablePath))
	{
		if (UDataTable* Table = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *TablePath)))
		{
			OnsetDialogueLibraryInternal::CachedDialogueTable = TStrongObjectPtr<UDataTable>(Table);
			return Table;
		}
	}
	return nullptr;
}

UDataTable* UOnsetDialogueLibrary::GetNPCTable()
{
	if (OnsetDialogueLibraryInternal::CachedNPCTable)
	{
		return OnsetDialogueLibraryInternal::CachedNPCTable.Get();
	}

	const FString TablePath = GetNPCTablePath();
	if (FPackageName::DoesPackageExist(TablePath))
	{
		if (UDataTable* Table = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *TablePath)))
		{
			OnsetDialogueLibraryInternal::CachedNPCTable = TStrongObjectPtr<UDataTable>(Table);
			return Table;
		}
	}
	return nullptr;
}

const FOnsetDialogueRow* UOnsetDialogueLibrary::GetDialogueRow(FName RowName)
{
	if (RowName.IsNone())
	{
		return nullptr;
	}

	OnsetDialogueLibraryInternal::EnsureDefaultFallbacks();

	if (UDataTable* Table = GetDialogueTable())
	{
		if (const FOnsetDialogueRow* Row = Table->FindRow<FOnsetDialogueRow>(RowName, nullptr))
		{
			return Row;
		}
	}

	return OnsetDialogueLibraryInternal::FallbackDialogueRows.Find(RowName);
}

const FOnsetNPCDefinition* UOnsetDialogueLibrary::GetNPCDefinition(FName RowName)
{
	if (RowName.IsNone())
	{
		return nullptr;
	}

	OnsetDialogueLibraryInternal::EnsureDefaultFallbacks();

	if (UDataTable* Table = GetNPCTable())
	{
		if (const FOnsetNPCDefinition* Def = Table->FindRow<FOnsetNPCDefinition>(RowName, nullptr))
		{
			return Def;
		}
	}

	return OnsetDialogueLibraryInternal::FallbackNPCDefinitions.Find(RowName);
}

void UOnsetDialogueLibrary::RegisterFallbackDialogueRow(FName RowName, const FOnsetDialogueRow& Row)
{
	OnsetDialogueLibraryInternal::EnsureDefaultFallbacks();
	if (!RowName.IsNone())
	{
		OnsetDialogueLibraryInternal::FallbackDialogueRows.Add(RowName, Row);
	}
}

void UOnsetDialogueLibrary::RegisterFallbackNPCDefinition(FName RowName, const FOnsetNPCDefinition& Definition)
{
	OnsetDialogueLibraryInternal::EnsureDefaultFallbacks();
	if (!RowName.IsNone())
	{
		OnsetDialogueLibraryInternal::FallbackNPCDefinitions.Add(RowName, Definition);
	}
}

bool UOnsetDialogueLibrary::IsChoiceAvailable(const FOnsetDialogueChoice& Choice, const UOnsetQuestComponent* QuestComponent)
{
	if (!Choice.RequiredQuestNotStarted.IsNone())
	{
		if (QuestComponent && QuestComponent->HasQuest(Choice.RequiredQuestNotStarted))
		{
			return false;
		}
	}

	if (!Choice.RequiredQuestActive.IsNone())
	{
		if (!QuestComponent || !QuestComponent->IsQuestActive(Choice.RequiredQuestActive))
		{
			return false;
		}
	}

	if (!Choice.RequiredQuestComplete.IsNone())
	{
		if (!QuestComponent || !QuestComponent->IsQuestComplete(Choice.RequiredQuestComplete))
		{
			return false;
		}
	}

	return true;
}

FName UOnsetDialogueLibrary::ResolveEntryDialogueRow(const FOnsetNPCDefinition& NPCDef, const UOnsetQuestComponent* QuestComponent)
{
	if (QuestComponent)
	{
		for (const FOnsetNPCQuestDialogueOverride& Override : NPCDef.QuestDialogueOverrides)
		{
			if (Override.QuestRowName.IsNone())
			{
				continue;
			}

			if (QuestComponent->IsQuestComplete(Override.QuestRowName))
			{
				if (!Override.CompletedDialogueRow.IsNone())
				{
					return Override.CompletedDialogueRow;
				}
			}
			else if (QuestComponent->IsQuestActive(Override.QuestRowName))
			{
				if (!Override.InProgressDialogueRow.IsNone())
				{
					return Override.InProgressDialogueRow;
				}
			}
			else
			{
				if (!Override.NotStartedDialogueRow.IsNone())
				{
					return Override.NotStartedDialogueRow;
				}
			}
		}
	}

	return NPCDef.DefaultDialogueRow;
}
