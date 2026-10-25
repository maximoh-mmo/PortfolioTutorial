// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/OnsetQuestTrackerWidget.h"

#include "CommonTextBlock.h"
#include "Components/VerticalBox.h"
#include "Data/OnsetQuestLibrary.h"
#include "Quest/UOnsetQuestComponent.h"

void UOnsetQuestTrackerWidget::BindToQuestComponent(UOnsetQuestComponent* InQuestComponent)
{
	if (BoundQuestComp.IsValid())
	{
		BoundQuestComp->OnQuestsChanged.RemoveAll(this);
	}

	BoundQuestComp = InQuestComponent;

	if (BoundQuestComp.IsValid())
	{
		BoundQuestComp->OnQuestsChanged.AddUObject(this, &UOnsetQuestTrackerWidget::HandleQuestsChanged);
		RefreshQuestList();
	}
}

void UOnsetQuestTrackerWidget::NativeDestruct()
{
	if (BoundQuestComp.IsValid())
	{
		BoundQuestComp->OnQuestsChanged.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UOnsetQuestTrackerWidget::HandleQuestsChanged()
{
	RefreshQuestList();
}

void UOnsetQuestTrackerWidget::RefreshQuestList()
{
	if (!BoundQuestComp.IsValid() || !QuestListContainer)
	{
		return;
	}

	QuestListContainer->ClearChildren();

	const TArray<FOnsetQuestState> Quests = BoundQuestComp->GetActiveQuests();
	int32 ActiveCount = 0;

	for (const FOnsetQuestState& State : Quests)
	{
		if (State.bComplete)
		{
			continue;
		}

		const FOnsetQuestDefinition* Def = UOnsetQuestLibrary::GetQuest(State.QuestRowName);
		if (!Def)
		{
			continue;
		}

		++ActiveCount;

		// Quest Title
		UCommonTextBlock* TitleBlock = NewObject<UCommonTextBlock>(this);
		if (TitleBlock)
		{
			const FText Title = !Def->DisplayName.IsEmpty() ? Def->DisplayName : FText::FromName(State.QuestRowName);
			TitleBlock->SetText(Title);
			TitleBlock->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.85f, 0.2f, 1.0f))); // Gold
			QuestListContainer->AddChildToVerticalBox(TitleBlock);
		}

		// Active Stage Description / Objectives
		if (Def->Stages.IsValidIndex(State.StageIndex))
		{
			const FOnsetQuestStageDefinition& Stage = Def->Stages[State.StageIndex];
			if (!Stage.Description.IsEmpty())
			{
				UCommonTextBlock* StageDesc = NewObject<UCommonTextBlock>(this);
				if (StageDesc)
				{
					StageDesc->SetText(Stage.Description);
					StageDesc->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f)));
					QuestListContainer->AddChildToVerticalBox(StageDesc);
				}
			}

			for (int32 ObjIdx = 0; ObjIdx < Stage.Objectives.Num(); ++ObjIdx)
			{
				const FOnsetQuestObjectiveDefinition& ObjDef = Stage.Objectives[ObjIdx];
				int32 CurrentProgress = 0;
				if (State.Stages.IsValidIndex(State.StageIndex) && State.Stages[State.StageIndex].Objectives.IsValidIndex(ObjIdx))
				{
					CurrentProgress = State.Stages[State.StageIndex].Objectives[ObjIdx].Progress;
				}
				const int32 TargetCount = FMath::Max(1, ObjDef.Count);

				UCommonTextBlock* ObjText = NewObject<UCommonTextBlock>(this);
				if (ObjText)
				{
					FText ObjDesc = ObjDef.Description;
					if (ObjDesc.IsEmpty())
					{
						ObjDesc = FText::Format(
							NSLOCTEXT("OnsetQuest", "DefaultObjFmt", "Progress: {0} / {1}"),
							FText::AsNumber(CurrentProgress),
							FText::AsNumber(TargetCount)
						);
					}
					else
					{
						ObjDesc = FText::Format(
							NSLOCTEXT("OnsetQuest", "NamedObjFmt", "{0} ({1}/{2})"),
							ObjDesc,
							FText::AsNumber(CurrentProgress),
							FText::AsNumber(TargetCount)
						);
					}

					ObjText->SetText(ObjDesc);
					const FLinearColor ObjColor = (CurrentProgress >= TargetCount)
						? FLinearColor(0.2f, 0.85f, 0.3f, 1.0f) // Green completed
						: FLinearColor(0.9f, 0.9f, 0.9f, 0.85f);
					ObjText->SetColorAndOpacity(FSlateColor(ObjColor));
					QuestListContainer->AddChildToVerticalBox(ObjText);
				}
			}
		}
	}

	BP_OnQuestTrackerUpdated(ActiveCount);
}
