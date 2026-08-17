// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/LootOverlayWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Data/OnsetItemLibrary.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"
#include "TimerManager.h"

/** Rarity-tinted text color so loot rarity reads at a glance. */
static FSlateColor GetRarityColor(EOnsetItemRarity Rarity)
{
	switch (Rarity)
	{
		case EOnsetItemRarity::Uncommon:	return FSlateColor(FLinearColor(0.3f, 1.0f, 0.4f, 1.0f));
		case EOnsetItemRarity::Rare:		return FSlateColor(FLinearColor(0.35f, 0.7f, 1.0f, 1.0f));
		case EOnsetItemRarity::Epic:		return FSlateColor(FLinearColor(0.75f, 0.35f, 1.0f, 1.0f));
		case EOnsetItemRarity::Legendary:	return FSlateColor(FLinearColor(1.0f, 0.6f, 0.2f, 1.0f));
		default:							return FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	}
}

void ULootOverlayWidget::ShowLoot(const TArray<FOnsetInventoryEntry>& LootedItems)
{
	if (LootedItems.Num() == 0)
	{
		return;
	}

	if (!ItemList)
	{
		BuildWidgetTree();
	}
	if (!ItemList)
	{
		return;
	}

	ItemList->ClearChildren();

	for (const FOnsetInventoryEntry& Entry : LootedItems)
	{
		const FOnsetItemDefinition* Def = UOnsetItemLibrary::GetItemDefinition(Entry.Category, Entry.RowName);
		FString Name = (Def && !Def->DisplayName.IsEmpty()) ? Def->DisplayName.ToString() : Entry.RowName.ToString();
		const FText Label = Entry.Count > 1
			? FText::Format(NSLOCTEXT("LootOverlay", "ItemLabel", "{0}  x{1}"), FText::FromString(Name), FText::AsNumber(Entry.Count))
			: FText::FromString(Name);

		UTextBlock* Row = WidgetTree
			? WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass())
			: NewObject<UTextBlock>(this);
		Row->SetText(Label);
		Row->SetJustification(ETextJustify::Center);
		Row->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 14));
		Row->SetColorAndOpacity(GetRarityColor(Def ? Def->Rarity : EOnsetItemRarity::Common));
		ItemList->AddChildToVerticalBox(Row);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimerHandle);
		World->GetTimerManager().SetTimer(HideTimerHandle, this, &ULootOverlayWidget::HideLoot, Lifetime, false);
	}
}

void ULootOverlayWidget::BuildWidgetTree()
{
	if (!WidgetTree || ItemList)
	{
		return;
	}

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	// Fill spacer pushes the panel toward the bottom of the screen.
	USpacer* Spacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	UVerticalBoxSlot* SpacerSlot = Root->AddChildToVerticalBox(Spacer);
	SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// Tinted, rounded-back panel with the item rows.
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	FSlateBrush Background = *FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	Background.TintColor = FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.65f));
	Panel->SetBrush(Background);
	Panel->SetPadding(FMargin(16.0f, 8.0f));

	ItemList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(ItemList);

	UVerticalBoxSlot* PanelSlot = Root->AddChildToVerticalBox(Panel);
	PanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	PanelSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Bottom);
	PanelSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
	PanelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 48.0f));

	WidgetTree->RootWidget = Root;
	SetVisibility(ESlateVisibility::Collapsed);
}

void ULootOverlayWidget::HideLoot()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void ULootOverlayWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimerHandle);
	}
	Super::NativeDestruct();
}
