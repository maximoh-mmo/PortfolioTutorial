// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/OnsetInteractionPromptWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"

void UOnsetInteractionPromptWidget::ShowPrompt(const FMHInteractionDefinition& Definition)
{
	if (!PromptText)
	{
		BuildWidgetTree();
	}
	if (!PromptText)
	{
		return;
	}

	const FText Action = Definition.ActionPrompt.IsEmpty()
		? NSLOCTEXT("OnsetInteraction", "DefaultAction", "Interact")
		: Definition.ActionPrompt;

	const FText Formatted = Definition.DisplayName.IsEmpty()
		? FText::Format(NSLOCTEXT("OnsetInteraction", "PromptFormatSimple", "[E] {0}"), Action)
		: FText::Format(NSLOCTEXT("OnsetInteraction", "PromptFormatNamed", "[E] {0}  —  {1}"), Action, Definition.DisplayName);

	PromptText->SetText(Formatted);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UOnsetInteractionPromptWidget::HidePrompt()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UOnsetInteractionPromptWidget::BuildWidgetTree()
{
	if (!WidgetTree || PromptText)
	{
		return;
	}

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	USpacer* Spacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	UVerticalBoxSlot* SpacerSlot = Root->AddChildToVerticalBox(Spacer);
	SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	FSlateBrush Background = *FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	Background.TintColor = FSlateColor(FLinearColor(0.04f, 0.05f, 0.08f, 0.82f));
	Panel->SetBrush(Background);
	Panel->SetPadding(FMargin(18.0f, 8.0f));

	PromptText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	PromptText->SetJustification(ETextJustify::Center);
	PromptText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 13));
	PromptText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.88f, 0.65f, 1.0f)));
	Panel->SetContent(PromptText);

	UVerticalBoxSlot* PanelSlot = Root->AddChildToVerticalBox(Panel);
	PanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	PanelSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Bottom);
	PanelSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
	PanelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 128.0f));

	WidgetTree->RootWidget = Root;
	SetVisibility(ESlateVisibility::Collapsed);
}
