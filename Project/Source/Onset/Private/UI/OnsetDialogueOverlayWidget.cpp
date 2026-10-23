// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/OnsetDialogueOverlayWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Data/OnsetDialogueLibrary.h"
#include "GameFramework/Pawn.h"
#include "Player/OnsetPlayerController.h"
#include "Quest/UOnsetQuestComponent.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"

void UOnsetDialogueChoiceButtonRelay::HandleButtonClicked()
{
	if (OwnerWidget.IsValid())
	{
		OwnerWidget->HandleChoiceSelected(ChoiceIndex);
	}
}

void UOnsetDialogueOverlayWidget::ShowDialogueNode(AActor* TargetNPC, FName DialogueRowName)
{
	const FOnsetDialogueRow* Row = UOnsetDialogueLibrary::GetDialogueRow(DialogueRowName);
	if (!Row)
	{
		CloseDialogue();
		return;
	}

	if (!SpeakerNameText || !DialogueBodyText || !ChoicesBox)
	{
		BuildWidgetTree();
	}
	if (!SpeakerNameText || !DialogueBodyText || !ChoicesBox)
	{
		return;
	}

	CurrentNPCActor = TargetNPC;
	CurrentDialogueRow = DialogueRowName;

	SpeakerNameText->SetText(Row->SpeakerName.IsEmpty() ? FText::FromString(GetNameSafe(TargetNPC)) : Row->SpeakerName);
	DialogueBodyText->SetText(Row->DialogueText);

	ChoicesBox->ClearChildren();
	ActiveRelays.Reset();

	const APlayerController* OwningPC = GetOwningPlayer();
	const APawn* OwningPawn = OwningPC ? OwningPC->GetPawn() : nullptr;
	const UOnsetQuestComponent* QuestComp = OwningPawn ? OwningPawn->FindComponentByClass<UOnsetQuestComponent>() : nullptr;

	int32 AddedChoices = 0;
	for (int32 i = 0; i < Row->Choices.Num(); ++i)
	{
		const FOnsetDialogueChoice& Choice = Row->Choices[i];
		if (!UOnsetDialogueLibrary::IsChoiceAvailable(Choice, QuestComp))
		{
			continue;
		}

		const bool bIsQuestAction = !Choice.GrantQuestOnSelect.IsNone() || !Choice.CompleteQuestOnSelect.IsNone();
		AddChoiceButton(Choice.ResponseText, i, bIsQuestAction);
		AddedChoices++;
	}

	if (AddedChoices == 0)
	{
		AddChoiceButton(NSLOCTEXT("OnsetDialogue", "DefaultClose", "Farewell."), -1, false);
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UOnsetDialogueOverlayWidget::CloseDialogue()
{
	CurrentNPCActor.Reset();
	CurrentDialogueRow = NAME_None;
	ActiveRelays.Reset();
	SetVisibility(ESlateVisibility::Collapsed);
}

bool UOnsetDialogueOverlayWidget::IsDialogueOpen() const
{
	return GetVisibility() != ESlateVisibility::Collapsed && GetVisibility() != ESlateVisibility::Hidden;
}

void UOnsetDialogueOverlayWidget::HandleChoiceSelected(int32 ChoiceIndex)
{
	if (ChoiceIndex < 0 || !CurrentNPCActor.IsValid() || CurrentDialogueRow.IsNone())
	{
		CloseDialogue();
		return;
	}

	if (AOnsetPlayerController* PC = Cast<AOnsetPlayerController>(GetOwningPlayer()))
	{
		PC->Server_SelectDialogueChoice(CurrentNPCActor.Get(), CurrentDialogueRow, ChoiceIndex);
	}
	else
	{
		CloseDialogue();
	}
}

void UOnsetDialogueOverlayWidget::AddChoiceButton(const FText& Label, int32 ChoiceIndex, bool bIsQuestAction)
{
	if (!ChoicesBox)
	{
		return;
	}

	UButton* Button = WidgetTree
		? WidgetTree->ConstructWidget<UButton>(UButton::StaticClass())
		: NewObject<UButton>(this);

	UTextBlock* ButtonText = WidgetTree
		? WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass())
		: NewObject<UTextBlock>(this);

	ButtonText->SetText(Label);
	ButtonText->SetFont(FCoreStyle::GetDefaultFontStyle(bIsQuestAction ? TEXT("Bold") : TEXT("Regular"), 13));
	ButtonText->SetColorAndOpacity(bIsQuestAction
		? FSlateColor(FLinearColor(1.0f, 0.85f, 0.35f, 1.0f))
		: FSlateColor(FLinearColor(0.92f, 0.94f, 0.98f, 1.0f)));

	Button->SetBackgroundColor(FLinearColor(0.15f, 0.18f, 0.24f, 0.9f));
	Button->SetContent(ButtonText);

	UOnsetDialogueChoiceButtonRelay* Relay = NewObject<UOnsetDialogueChoiceButtonRelay>(this);
	Relay->OwnerWidget = this;
	Relay->ChoiceIndex = ChoiceIndex;
	ActiveRelays.Add(Relay);

	Button->OnClicked.AddDynamic(Relay, &UOnsetDialogueChoiceButtonRelay::HandleButtonClicked);

	if (UVerticalBoxSlot* ButtonSlot = ChoicesBox->AddChildToVerticalBox(Button))
	{
		ButtonSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 4.0f));
	}
}

void UOnsetDialogueOverlayWidget::BuildWidgetTree()
{
	if (!WidgetTree || (SpeakerNameText && DialogueBodyText && ChoicesBox))
	{
		return;
	}

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	USpacer* TopSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	if (UVerticalBoxSlot* TopSlot = Root->AddChildToVerticalBox(TopSpacer))
	{
		TopSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	USizeBox* BoxSizer = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	BoxSizer->SetWidthOverride(640.0f);

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	FSlateBrush Background = *FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	Background.TintColor = FSlateColor(FLinearColor(0.03f, 0.04f, 0.07f, 0.92f));
	Panel->SetBrush(Background);
	Panel->SetPadding(FMargin(24.0f, 18.0f));

	UVerticalBox* ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	SpeakerNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	SpeakerNameText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 16));
	SpeakerNameText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.82f, 0.4f, 1.0f)));
	if (UVerticalBoxSlot* SpeakerSlot = ContentBox->AddChildToVerticalBox(SpeakerNameText))
	{
		SpeakerSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	DialogueBodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	DialogueBodyText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 14));
	DialogueBodyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.92f, 0.95f, 1.0f)));
	DialogueBodyText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* BodySlot = ContentBox->AddChildToVerticalBox(DialogueBodyText))
	{
		BodySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	}

	ChoicesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	ContentBox->AddChildToVerticalBox(ChoicesBox);

	Panel->SetContent(ContentBox);
	BoxSizer->SetContent(Panel);

	if (UVerticalBoxSlot* PanelSlot = Root->AddChildToVerticalBox(BoxSizer))
	{
		PanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		PanelSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Bottom);
		PanelSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
		PanelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 160.0f));
	}

	WidgetTree->RootWidget = Root;
	SetVisibility(ESlateVisibility::Collapsed);
}
