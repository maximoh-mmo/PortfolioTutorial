// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/OnsetItemEditorWidget.h"

#include "Combat/OnsetAbilityLibrary.h"
#include "Combat/OnsetLootLibrary.h"
#include "Components/Button.h"
#include "Components/DetailsView.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Data/OnsetItemLibrary.h"
#include "Data/OnsetEquipmentTypes.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "OnsetEditor.h"
#include "PackageTools.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "Styling/StyleDefaults.h"
#include "UI/OnsetAbilityRowButton.h"
#include "UI/OnsetNotifyDetailsView.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

namespace OnsetItemEditorWidgetConstants
{
	constexpr float ListEntryHeight = 22.0f;
	constexpr float NameWidth = 180.0f;
	constexpr float RarityWidth = 90.0f;
	constexpr float LevelWidth = 60.0f;
}

/** Generates a unique widget name so WidgetTree names never collide across rows. */
static FName ItemNextWidgetName(const FString& Base)
{
	static int32 WidgetCounter = 0;
	return FName(*FString::Printf(TEXT("%s_%d"), *Base, ++WidgetCounter));
}

/** Flat, compact button style: no chrome, subtle hover/press fill. */
static FButtonStyle MakeItemFlatStyle()
{
	FButtonStyle Style;
	Style.SetNormal(*FStyleDefaults::GetNoBrush());

	FSlateBrush Hover = *FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	Hover.TintColor = FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.10f));
	FSlateBrush Pressed = Hover;
	Pressed.TintColor = FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.18f));

	Style.SetHovered(Hover);
	Style.SetPressed(Pressed);
	Style.SetDisabled(*FStyleDefaults::GetNoBrush());
	Style.SetNormalPadding(FMargin(4.0f, 1.0f));
	Style.SetPressedPadding(FMargin(4.0f, 2.0f));
	return Style;
}

/** Row style: transparent normally; tinted fill when the row is selected. */
static FButtonStyle MakeItemRowStyle(bool bSelected)
{
	FButtonStyle Style = MakeItemFlatStyle();
	Style.SetNormalPadding(FMargin(0.0f, 1.0f));
	Style.SetPressedPadding(FMargin(0.0f, 2.0f));
	if (bSelected)
	{
		FSlateBrush Selected = *FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
		Selected.TintColor = FSlateColor(FLinearColor(0.15f, 0.35f, 0.75f, 0.35f));
		FSlateBrush SelectedHover = Selected;
		SelectedHover.TintColor = FSlateColor(FLinearColor(0.20f, 0.40f, 0.80f, 0.45f));
		Style.SetNormal(Selected);
		Style.SetHovered(SelectedHover);
		Style.SetPressed(SelectedHover);
	}
	return Style;
}

/** A fixed-width text cell (used for both the column header and row values). */
static USizeBox* MakeItemCell(UWidgetTree* WidgetTree, const FString& BaseName, float Width, const FText& Text, bool bHeader, ETextJustify::Type Justification)
{
	USizeBox* CellBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), ItemNextWidgetName(BaseName + TEXT("_Cell")));
	CellBox->SetWidthOverride(Width);

	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), ItemNextWidgetName(BaseName + TEXT("_Label")));
	TextBlock->SetText(Text);
	TextBlock->SetJustification(Justification);
	TextBlock->SetFont(bHeader ? FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 10) : FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10));
	if (bHeader)
	{
		TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.65f, 0.65f, 1.0f)));
	}

	CellBox->AddChild(TextBlock);
	return CellBox;
}

/** Creates a compact UButton whose content is a single centered text block. */
static UButton* MakeItemTextButton(UWidgetTree* WidgetTree, const FText& Label, const FString& BaseName)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ItemNextWidgetName(BaseName + TEXT("_Button")));
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), ItemNextWidgetName(BaseName + TEXT("_Label")));
	Text->SetText(Label);
	Text->SetJustification(ETextJustify::Center);
	Text->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10));
	Button->SetContent(Text);
	Button->SetStyle(MakeItemFlatStyle());
	return Button;
}

UDataTable* UOnsetItemEditorWidget::GetTableForMode(EOnsetItemEditorMode Mode) const
{
	switch (Mode)
	{
		case EOnsetItemEditorMode::Equipment:	return UOnsetItemLibrary::GetTable(EOnsetItemCategory::Equipment);
		case EOnsetItemEditorMode::QuestItem:	return UOnsetItemLibrary::GetTable(EOnsetItemCategory::QuestItem);
		case EOnsetItemEditorMode::Junk:			return UOnsetItemLibrary::GetTable(EOnsetItemCategory::Junk);
		case EOnsetItemEditorMode::Scroll:		return UOnsetItemLibrary::GetTable(EOnsetItemCategory::Scroll);
		case EOnsetItemEditorMode::LootTable:	return UOnsetLootLibrary::GetLootTable();
	}
	return nullptr;
}

const FOnsetItemDefinition* UOnsetItemEditorWidget::FindItemBase(EOnsetItemCategory Category, FName RowName) const
{
	if (RowName.IsNone())
	{
		return nullptr;
	}
	UDataTable* Table = UOnsetItemLibrary::GetTable(Category);
	if (!Table)
	{
		return nullptr;
	}

	switch (Category)
	{
		case EOnsetItemCategory::Equipment:	return Table->FindRow<FOnsetEquipmentDefinition>(RowName, nullptr);
		case EOnsetItemCategory::QuestItem:	return Table->FindRow<FOnsetQuestItemDefinition>(RowName, nullptr);
		case EOnsetItemCategory::Junk:		return Table->FindRow<FOnsetJunkItemDefinition>(RowName, nullptr);
		case EOnsetItemCategory::Scroll:	return Table->FindRow<FOnsetScrollDefinition>(RowName, nullptr);
	}
	return nullptr;
}

void UOnsetItemEditorWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildUI();
	RefreshFromTable();
}

void UOnsetItemEditorWidget::NativeDestruct()
{
	PersistOnClose();
	Super::NativeDestruct();
}

void UOnsetItemEditorWidget::OpenEditor()
{
	BuildUI();
	RefreshFromTable();
}

void UOnsetItemEditorWidget::BuildUI()
{
	if (WidgetTree && WidgetTree->RootWidget)
	{
		return;
	}

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootLayout"));

	// --- Top: mode tabs ---
	UHorizontalBox* ModeRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ModeRow"));
	const TArray<TPair<EOnsetItemEditorMode, FText>> Modes = {
		{ EOnsetItemEditorMode::Equipment, NSLOCTEXT("OnsetItemEditor", "ModeEquipment", "Equipment") },
		{ EOnsetItemEditorMode::QuestItem, NSLOCTEXT("OnsetItemEditor", "ModeQuest", "Quest Items") },
		{ EOnsetItemEditorMode::Junk, NSLOCTEXT("OnsetItemEditor", "ModeJunk", "Junk") },
		{ EOnsetItemEditorMode::Scroll, NSLOCTEXT("OnsetItemEditor", "ModeScroll", "Scrolls") },
		{ EOnsetItemEditorMode::LootTable, NSLOCTEXT("OnsetItemEditor", "ModeLoot", "Loot Tables") },
	};

	ModeButtons.Reset();
	for (const TPair<EOnsetItemEditorMode, FText>& Mode : Modes)
	{
		UOnsetItemModeButton* ModeButton = NewObject<UOnsetItemModeButton>(this);
		ModeButton->Mode = Mode.Key;
		ModeButton->SetStyle(MakeItemFlatStyle());

		UTextBlock* ModeLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), ItemNextWidgetName(TEXT("ModeLabel")));
		ModeLabel->SetText(Mode.Value);
		ModeLabel->SetJustification(ETextJustify::Center);
		ModeLabel->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10));
		ModeButton->SetContent(ModeLabel);
		ModeButton->SetToolTipText(Mode.Value);

		ModeButton->OnClicked.AddDynamic(ModeButton, &UOnsetItemModeButton::HandleClicked);
		ModeButton->OnModeSelected.AddUObject(this, &UOnsetItemEditorWidget::OnModeButtonClicked);

		ModeRow->AddChildToHorizontalBox(ModeButton)->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));
		ModeButtons.Add(ModeButton);
	}
	RootBox->AddChildToVerticalBox(ModeRow)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));

	// --- Middle: list + details ---
	UHorizontalBox* ContentRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ContentRow"));

	UVerticalBox* LeftPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftPanel"));

	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ColumnHeader"));
	HeaderRow->AddChild(MakeItemCell(WidgetTree, TEXT("HeaderName"), OnsetItemEditorWidgetConstants::NameWidth, NSLOCTEXT("OnsetItemEditor", "ColumnName", "Name"), true, ETextJustify::Left));
	HeaderRow->AddChild(MakeItemCell(WidgetTree, TEXT("HeaderRarity"), OnsetItemEditorWidgetConstants::RarityWidth, NSLOCTEXT("OnsetItemEditor", "ColumnRarity", "Rarity"), true, ETextJustify::Center));
	HeaderRow->AddChild(MakeItemCell(WidgetTree, TEXT("HeaderLevel"), OnsetItemEditorWidgetConstants::LevelWidth, NSLOCTEXT("OnsetItemEditor", "ColumnLevel", "Level"), true, ETextJustify::Center));
	LeftPanel->AddChildToVerticalBox(HeaderRow);

	UScrollBox* ListScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RowList"));
	ListScroll->SetAlwaysShowScrollbar(true);
	ListScroll->SetAlwaysShowScrollbarTrack(true);
	LeftPanel->AddChildToVerticalBox(ListScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	RowListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RowListBox"));
	ListScroll->AddChild(RowListBox);

	PropertyView = WidgetTree->ConstructWidget<UOnsetNotifyDetailsView>(UOnsetNotifyDetailsView::StaticClass(), TEXT("PropertyView"));
	PropertyView->OnPropertyEdited.AddUObject(this, &UOnsetItemEditorWidget::HandlePropertyEdited);

	ContentRow->AddChildToHorizontalBox(LeftPanel)->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	ContentRow->AddChildToHorizontalBox(PropertyView)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	RootBox->AddChildToVerticalBox(ContentRow)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// --- Status line (validation / roll-preview results) ---
	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	StatusText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10));
	StatusText->SetAutoWrapText(true);
	RootBox->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));

	// --- Bottom: action bar ---
	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonRow"));

	UButton* AddButton = MakeItemTextButton(WidgetTree, NSLOCTEXT("OnsetItemEditor", "Add", "Add"), TEXT("Add"));
	AddButton->OnClicked.AddDynamic(this, &UOnsetItemEditorWidget::AddDefinition);
	ButtonRow->AddChildToHorizontalBox(AddButton)->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));

	UButton* DeleteButton = MakeItemTextButton(WidgetTree, NSLOCTEXT("OnsetItemEditor", "Delete", "Delete"), TEXT("Delete"));
	DeleteButton->OnClicked.AddDynamic(this, &UOnsetItemEditorWidget::DeleteDefinition);
	ButtonRow->AddChildToHorizontalBox(DeleteButton)->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));

	UButton* RollButton = MakeItemTextButton(WidgetTree, NSLOCTEXT("OnsetItemEditor", "RollPreview", "Roll Preview (loot)"), TEXT("Roll"));
	RollButton->OnClicked.AddDynamic(this, &UOnsetItemEditorWidget::RollPreview);
	ButtonRow->AddChildToHorizontalBox(RollButton);

	UVerticalBoxSlot* ButtonRowSlot = RootBox->AddChildToVerticalBox(ButtonRow);
	ButtonRowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	ButtonRowSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
	ButtonRowSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));

	WidgetTree->RootWidget = RootBox;
	UpdateModeButtonHighlight();
}

void UOnsetItemEditorWidget::UpdateModeButtonHighlight()
{
	for (UOnsetItemModeButton* ModeButton : ModeButtons)
	{
		if (ModeButton)
		{
			const bool bActive = (ModeButton->Mode == CurrentMode);
			ModeButton->SetStyle(bActive ? MakeItemRowStyle(true) : MakeItemFlatStyle());
		}
	}
}

void UOnsetItemEditorWidget::SetMode(EOnsetItemEditorMode NewMode)
{
	if (NewMode == CurrentMode)
	{
		return;
	}

	// Commit any pending edits of the current mode before switching tables.
	WriteBackRow();

	// Detach the wrapper for the old mode and switch.
	CurrentMode = NewMode;
	SelectedRowName = NAME_None;
	UpdateModeButtonHighlight();

	if (PropertyView)
	{
		PropertyView->SetObject(nullptr);
	}
	if (StatusText)
	{
		StatusText->SetText(FText::GetEmpty());
	}

	RefreshFromTable();
}

void UOnsetItemEditorWidget::RefreshFromTable()
{
	RebuildList();
}

void UOnsetItemEditorWidget::RebuildList()
{
	if (!RowListBox)
	{
		return;
	}

	RowListBox->ClearChildren();
	RowButtons.Reset();

	UDataTable* Table = GetTableForMode(CurrentMode);
	if (!Table)
	{
		return;
	}

	UEnum* RarityEnum = StaticEnum<EOnsetItemRarity>();

	for (const TPair<FName, uint8*>& RowPair : Table->GetRowMap())
	{
		FName RowName = RowPair.Key;

		FText NameText;
		FText RarityText;
		FText LevelText;

		if (CurrentMode == EOnsetItemEditorMode::LootTable)
		{
			const FOnsetLootTableRow* Loot = reinterpret_cast<const FOnsetLootTableRow*>(RowPair.Value);
			if (!Loot)
			{
				continue;
			}
			NameText = FText::FromName(RowName);
			RarityText = FText::AsNumber(Loot->Entries.Num());
			LevelText = FText::AsNumber(Loot->SubTables.Num());
		}
		else
		{
			const FOnsetItemDefinition* Item = reinterpret_cast<const FOnsetItemDefinition*>(RowPair.Value);
			if (!Item)
			{
				continue;
			}
			NameText = Item->DisplayName.IsEmpty() ? FText::FromName(RowName) : Item->DisplayName;
			RarityText = RarityEnum ? RarityEnum->GetDisplayValueAsText(Item->Rarity) : FText::FromString(TEXT("?"));
			LevelText = FText::AsNumber(Item->LevelRequirement);
		}

		UOnsetAbilityRowButton* RowButton = NewObject<UOnsetAbilityRowButton>(this);
		RowButton->RowName = RowName;
		RowButton->SetStyle(MakeItemRowStyle(RowName == SelectedRowName));

		USizeBox* RowHeightBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), ItemNextWidgetName(TEXT("RowSize")));
		RowHeightBox->SetHeightOverride(OnsetItemEditorWidgetConstants::ListEntryHeight);

		UHorizontalBox* Cells = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), ItemNextWidgetName(TEXT("RowCells")));
		Cells->AddChild(MakeItemCell(WidgetTree, TEXT("Name"), OnsetItemEditorWidgetConstants::NameWidth, NameText, false, ETextJustify::Left));
		Cells->AddChild(MakeItemCell(WidgetTree, TEXT("Rarity"), OnsetItemEditorWidgetConstants::RarityWidth, RarityText, false, ETextJustify::Center));
		Cells->AddChild(MakeItemCell(WidgetTree, TEXT("Level"), OnsetItemEditorWidgetConstants::LevelWidth, LevelText, false, ETextJustify::Center));

		RowHeightBox->AddChild(Cells);
		RowButton->SetContent(RowHeightBox);

		RowButton->OnClicked.AddDynamic(RowButton, &UOnsetAbilityRowButton::HandleClicked);
		RowButton->OnRowSelected.AddUObject(this, &UOnsetItemEditorWidget::SelectRow);

		RowListBox->AddChildToVerticalBox(RowButton)->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Left);
		RowButtons.Add(RowName, RowButton);
	}
}

void UOnsetItemEditorWidget::SelectRow(FName RowName)
{
	// Commit any pending edits of the previously selected row before switching.
	WriteBackRow();

	UDataTable* Table = GetTableForMode(CurrentMode);
	if (!Table)
	{
		return;
	}

	SelectedRowName = RowName;

	// Ensure the wrapper for this mode exists, then fill it with the row.
	switch (CurrentMode)
	{
		case EOnsetItemEditorMode::Equipment:
		{
			if (const FOnsetEquipmentDefinition* Row = Table->FindRow<FOnsetEquipmentDefinition>(RowName, nullptr))
			{
				if (!EquipmentWrapper)
				{
					EquipmentWrapper = NewObject<UOnsetEquipmentRowWrapper>(this);
				}
				EquipmentWrapper->Row = *Row;
				if (PropertyView) PropertyView->SetObject(EquipmentWrapper);
			}
			break;
		}
		case EOnsetItemEditorMode::QuestItem:
		{
			if (const FOnsetQuestItemDefinition* Row = Table->FindRow<FOnsetQuestItemDefinition>(RowName, nullptr))
			{
				if (!QuestItemWrapper)
				{
					QuestItemWrapper = NewObject<UOnsetQuestItemRowWrapper>(this);
				}
				QuestItemWrapper->Row = *Row;
				if (PropertyView) PropertyView->SetObject(QuestItemWrapper);
			}
			break;
		}
		case EOnsetItemEditorMode::Junk:
		{
			if (const FOnsetJunkItemDefinition* Row = Table->FindRow<FOnsetJunkItemDefinition>(RowName, nullptr))
			{
				if (!JunkWrapper)
				{
					JunkWrapper = NewObject<UOnsetJunkRowWrapper>(this);
				}
				JunkWrapper->Row = *Row;
				if (PropertyView) PropertyView->SetObject(JunkWrapper);
			}
			break;
		}
		case EOnsetItemEditorMode::Scroll:
		{
			if (const FOnsetScrollDefinition* Row = Table->FindRow<FOnsetScrollDefinition>(RowName, nullptr))
			{
				if (!ScrollWrapper)
				{
					ScrollWrapper = NewObject<UOnsetScrollRowWrapper>(this);
				}
				ScrollWrapper->Row = *Row;
				if (PropertyView) PropertyView->SetObject(ScrollWrapper);
			}
			break;
		}
		case EOnsetItemEditorMode::LootTable:
		{
			if (const FOnsetLootTableRow* Row = Table->FindRow<FOnsetLootTableRow>(RowName, nullptr))
			{
				if (!LootWrapper)
				{
					LootWrapper = NewObject<UOnsetLootRowWrapper>(this);
				}
				LootWrapper->Row = *Row;
				if (PropertyView) PropertyView->SetObject(LootWrapper);
			}
			break;
		}
	}

	for (const TPair<FName, TObjectPtr<UOnsetAbilityRowButton>>& Pair : RowButtons)
	{
		if (Pair.Value)
		{
			Pair.Value->SetStyle(MakeItemRowStyle(Pair.Key == SelectedRowName));
		}
	}
}

bool UOnsetItemEditorWidget::WriteBackRow()
{
	UDataTable* Table = GetTableForMode(CurrentMode);
	if (!Table || SelectedRowName.IsNone())
	{
		return false;
	}

	switch (CurrentMode)
	{
		case EOnsetItemEditorMode::Equipment:
			if (EquipmentWrapper)
			{
				if (FOnsetEquipmentDefinition* Row = Table->FindRow<FOnsetEquipmentDefinition>(SelectedRowName, nullptr)) { *Row = EquipmentWrapper->Row; return true; }
			}
			break;
		case EOnsetItemEditorMode::QuestItem:
			if (QuestItemWrapper)
			{
				if (FOnsetQuestItemDefinition* Row = Table->FindRow<FOnsetQuestItemDefinition>(SelectedRowName, nullptr)) { *Row = QuestItemWrapper->Row; return true; }
			}
			break;
		case EOnsetItemEditorMode::Junk:
			if (JunkWrapper)
			{
				if (FOnsetJunkItemDefinition* Row = Table->FindRow<FOnsetJunkItemDefinition>(SelectedRowName, nullptr)) { *Row = JunkWrapper->Row; return true; }
			}
			break;
		case EOnsetItemEditorMode::Scroll:
			if (ScrollWrapper)
			{
				if (FOnsetScrollDefinition* Row = Table->FindRow<FOnsetScrollDefinition>(SelectedRowName, nullptr)) { *Row = ScrollWrapper->Row; return true; }
			}
			break;
		case EOnsetItemEditorMode::LootTable:
			if (LootWrapper)
			{
				if (FOnsetLootTableRow* Row = Table->FindRow<FOnsetLootTableRow>(SelectedRowName, nullptr)) { *Row = LootWrapper->Row; return true; }
			}
			break;
	}
	return false;
}

FString UOnsetItemEditorWidget::ValidateTable(const UDataTable* Table) const
{
	if (!Table)
	{
		return FString();
	}

	TArray<FString> Errors;

	if (Table == UOnsetLootLibrary::GetLootTable())
	{
		// Loot entries: every Item handle must resolve in its category table.
		// Sub-table refs: must point at DT_Loot rows, and must not cycle.
		for (const TPair<FName, uint8*>& RowPair : Table->GetRowMap())
		{
			const FName RowName = RowPair.Key;
			const FOnsetLootTableRow* Loot = reinterpret_cast<const FOnsetLootTableRow*>(RowPair.Value);
			if (!Loot)
			{
				continue;
			}

			for (const FOnsetLootEntry& Entry : Loot->Entries)
			{
				if (!Entry.Item.DataTable || Entry.Item.RowName.IsNone())
				{
					Errors.Add(FString::Printf(TEXT("Table '%s': an entry has an empty Item handle."), *RowName.ToString()));
					continue;
				}
				const EOnsetItemCategory Category = UOnsetItemLibrary::GetCategoryForTable(Entry.Item.DataTable);
				if (!FindItemBase(Category, Entry.Item.RowName))
				{
					Errors.Add(FString::Printf(TEXT("Entry '%s' -> item '%s' does not exist in its table."), *RowName.ToString(), *Entry.Item.RowName.ToString()));
				}
			}

			for (const FOnsetLootSubTableRef& Ref : Loot->SubTables)
			{
				if (!Ref.Table.DataTable || Ref.Table.RowName.IsNone())
				{
					Errors.Add(FString::Printf(TEXT("Sub-table in '%s' has an empty handle."), *RowName.ToString()));
					continue;
				}
				if (Ref.Table.DataTable != UOnsetLootLibrary::GetLootTable())
				{
					Errors.Add(FString::Printf(TEXT("Sub-table '%s' points at a non-loot table."), *Ref.Table.RowName.ToString()));
				}
			}
		}

		// Cycle check across all rows in the loot table.
		UDataTable* LootTable = UOnsetLootLibrary::GetLootTable();
		TSet<FName> Visiting;
		TSet<FName> Visited;
		TFunction<bool(FName)> DetectCycle = [&](FName TableRow) -> bool
		{
			if (Visited.Contains(TableRow)) return false;
			if (Visiting.Contains(TableRow)) return true;

			const FOnsetLootTableRow* Row = LootTable ? LootTable->FindRow<FOnsetLootTableRow>(TableRow, nullptr) : nullptr;
			if (!Row) return false;

			Visiting.Add(TableRow);
			for (const FOnsetLootSubTableRef& Ref : Row->SubTables)
			{
				if (!Ref.Table.DataTable || Ref.Table.RowName.IsNone()) continue;
				if (Ref.Table.DataTable != LootTable) continue; // cross-table refs validated above
				if (DetectCycle(Ref.Table.RowName))
				{
					return true;
				}
			}
			Visiting.Remove(TableRow);
			Visited.Add(TableRow);
			return false;
		};

		for (const TPair<FName, uint8*>& RowPair : Table->GetRowMap())
		{
			if (DetectCycle(RowPair.Key))
			{
				Errors.Add(FString::Printf(TEXT("Sub-table reference cycle detected for table '%s'."), *RowPair.Key.ToString()));
				break;
			}
		}
	}
	else if (Table == UOnsetItemLibrary::GetTable(EOnsetItemCategory::Scroll))
	{
		// Scrolls: every GrantedAbility must exist in DT_Abilities.
		UDataTable* AbilityTable = UOnsetAbilityLibrary::GetAbilityTable();
		for (const TPair<FName, uint8*>& RowPair : Table->GetRowMap())
		{
			const FOnsetScrollDefinition* Scroll = reinterpret_cast<const FOnsetScrollDefinition*>(RowPair.Value);
			if (!Scroll || !Scroll->GrantedAbility.DataTable || Scroll->GrantedAbility.RowName.IsNone())
			{
				continue;
			}
			if (Scroll->GrantedAbility.DataTable != AbilityTable || !AbilityTable->FindRow<FOnsetAbilityDefinition>(Scroll->GrantedAbility.RowName, nullptr))
			{
				Errors.Add(FString::Printf(TEXT("Scroll '%s' grants ability '%s' which does not exist in DT_Abilities."), *RowPair.Key.ToString(), *Scroll->GrantedAbility.RowName.ToString()));
			}
		}
	}

	return FString::Join(Errors, TEXT("\n"));
}

void UOnsetItemEditorWidget::AddDefinition()
{
	UDataTable* Table = GetTableForMode(CurrentMode);
	if (!Table)
	{
		return;
	}

	// Build a unique row name based on the mode.
	FString BaseName = TEXT("Item");
	switch (CurrentMode)
	{
		case EOnsetItemEditorMode::Equipment:	BaseName = TEXT("NewEquipment"); break;
		case EOnsetItemEditorMode::QuestItem:	BaseName = TEXT("NewQuestItem"); break;
		case EOnsetItemEditorMode::Junk:			BaseName = TEXT("NewJunk"); break;
		case EOnsetItemEditorMode::Scroll:		BaseName = TEXT("NewScroll"); break;
		case EOnsetItemEditorMode::LootTable:	BaseName = TEXT("NewLootTable"); break;
	}

	FName NewRowName = FName(*BaseName);
	int32 Suffix = 1;
	bool bExists = true;
	while (bExists)
	{
		bExists = false;
		switch (CurrentMode)
		{
			case EOnsetItemEditorMode::Equipment:	bExists = Table->FindRow<FOnsetEquipmentDefinition>(NewRowName, nullptr) != nullptr; break;
			case EOnsetItemEditorMode::QuestItem:	bExists = Table->FindRow<FOnsetQuestItemDefinition>(NewRowName, nullptr) != nullptr; break;
			case EOnsetItemEditorMode::Junk:		bExists = Table->FindRow<FOnsetJunkItemDefinition>(NewRowName, nullptr) != nullptr; break;
			case EOnsetItemEditorMode::Scroll:		bExists = Table->FindRow<FOnsetScrollDefinition>(NewRowName, nullptr) != nullptr; break;
			case EOnsetItemEditorMode::LootTable:	bExists = Table->FindRow<FOnsetLootTableRow>(NewRowName, nullptr) != nullptr; break;
		}
		if (bExists)
		{
			NewRowName = FName(*FString::Printf(TEXT("%s%d"), *BaseName, Suffix++));
		}
	}

	switch (CurrentMode)
	{
		case EOnsetItemEditorMode::Equipment:
		{
			FOnsetEquipmentDefinition Row;
			Row.DisplayName = FText::FromName(NewRowName);
			Table->AddRow(NewRowName, Row);
			break;
		}
		case EOnsetItemEditorMode::QuestItem:
		{
			FOnsetQuestItemDefinition Row;
			Row.DisplayName = FText::FromName(NewRowName);
			Table->AddRow(NewRowName, Row);
			break;
		}
		case EOnsetItemEditorMode::Junk:
		{
			FOnsetJunkItemDefinition Row;
			Row.DisplayName = FText::FromName(NewRowName);
			Table->AddRow(NewRowName, Row);
			break;
		}
		case EOnsetItemEditorMode::Scroll:
		{
			FOnsetScrollDefinition Row;
			Row.DisplayName = FText::FromName(NewRowName);
			Table->AddRow(NewRowName, Row);
			break;
		}
		case EOnsetItemEditorMode::LootTable:
		{
			FOnsetLootTableRow Row;
			Table->AddRow(NewRowName, Row);
			break;
		}
	}

	// Persist later when the editor closes (see PersistOnClose).
	Table->GetPackage()->MarkPackageDirty();
	DirtyModes.Add(CurrentMode);

	RefreshFromTable();
	SelectRow(NewRowName);
}

void UOnsetItemEditorWidget::DeleteDefinition()
{
	UDataTable* Table = GetTableForMode(CurrentMode);
	if (!Table || SelectedRowName.IsNone())
	{
		return;
	}

	Table->RemoveRow(SelectedRowName);
	SelectedRowName = NAME_None;
	if (PropertyView)
	{
		PropertyView->SetObject(nullptr);
	}
	if (StatusText)
	{
		StatusText->SetText(FText::GetEmpty());
	}

	// Persist later when the editor closes (see PersistOnClose).
	Table->GetPackage()->MarkPackageDirty();
	DirtyModes.Add(CurrentMode);

	RefreshFromTable();
}

void UOnsetItemEditorWidget::HandlePropertyEdited()
{
	// The user changed a property in the details panel: write the new value into
	// the in-memory table immediately so switching modes never loses the edit.
	WriteBackRow();
	DirtyModes.Add(CurrentMode);
}

void UOnsetItemEditorWidget::PersistOnClose()
{
	if (DirtyModes.Num() == 0)
	{
		return;
	}

	// Commit any pending edit of the currently selected row.
	WriteBackRow();

	TArray<UObject*> ToSave;
	TArray<FString> Blocked;

	for (const EOnsetItemEditorMode Mode : DirtyModes)
	{
		UDataTable* Table = GetTableForMode(Mode);
		if (!Table)
		{
			continue;
		}

		// Validate every row of this table before persisting.
		const FString ValidationErrors = ValidateTable(Table);
		if (!ValidationErrors.IsEmpty())
		{
			Blocked.Add(FString::Printf(TEXT("%s: %s"), *Table->GetName(), *ValidationErrors));
			continue;
		}

		ToSave.AddUnique(Table);
	}

	if (ToSave.Num() > 0)
	{
		for (UObject* Obj : ToSave)
		{
			if (Obj)
			{
				Obj->GetPackage()->MarkPackageDirty();
			}
		}
		UPackageTools::SavePackagesForObjects(ToSave);
	}

	if (Blocked.Num() > 0)
	{
		UpdateStatusText(FText::FromString(FString::Printf(TEXT("Persistence skipped (validation failed):\n%s"), *FString::Join(Blocked, TEXT("\n")))), FLinearColor(1.0f, 0.3f, 0.3f, 1.0f));
	}

	DirtyModes.Reset();
}

void UOnsetItemEditorWidget::RollPreview()
{
	if (CurrentMode != EOnsetItemEditorMode::LootTable || SelectedRowName.IsNone())
	{
		return;
	}

	UDataTable* LootTable = UOnsetLootLibrary::GetLootTable();
	if (!LootTable)
	{
		return;
	}

	FDataTableRowHandle Handle;
	Handle.DataTable = LootTable;
	Handle.RowName = SelectedRowName;

	constexpr int32 RollCount = 1000;
	FOnsetLootContext Context;
	Context.Level = 0;
	Context.ZoneTag = FGameplayTag();

	// Aggregate quantities by (category, rowname).
	TMap<FName, int32> TotalQuantities;
	for (int32 i = 0; i < RollCount; ++i)
	{
		const TArray<FOnsetInventoryEntry> Roll = UOnsetLootLibrary::RollLoot(Handle, Context);
		for (const FOnsetInventoryEntry& Entry : Roll)
		{
			TotalQuantities.FindOrAdd(Entry.RowName) += Entry.Count;
		}
	}

	if (TotalQuantities.Num() == 0)
	{
		UpdateStatusText(FText::Format(NSLOCTEXT("OnsetItemEditor", "NoDrops", "{0} rolls of '{1}' produced no drops."), RollCount, FText::FromName(SelectedRowName)), FLinearColor(1.0f, 0.8f, 0.3f, 1.0f));
		return;
	}

	TArray<TPair<FName, int32>> Sorted;
	for (const TPair<FName, int32>& Pair : TotalQuantities)
	{
		Sorted.Add(Pair);
	}
	Sorted.Sort([](const TPair<FName, int32>& A, const TPair<FName, int32>& B) { return A.Value > B.Value; });

	TArray<FString> Lines;
	Lines.Add(FString::Printf(TEXT("%d rolls of '%s' (no level/zone gating):"), RollCount, *SelectedRowName.ToString()));
	for (const TPair<FName, int32>& Pair : Sorted)
	{
		const FString DisplayName = [&]() -> FString
		{
			// Look up the item's display name across the category tables.
			const EOnsetItemCategory Categories[] = { EOnsetItemCategory::Equipment, EOnsetItemCategory::QuestItem, EOnsetItemCategory::Junk, EOnsetItemCategory::Scroll };
			for (EOnsetItemCategory Category : Categories)
			{
				if (const FOnsetItemDefinition* Item = FindItemBase(Category, Pair.Key))
				{
					return Item->DisplayName.IsEmpty() ? Pair.Key.ToString() : Item->DisplayName.ToString();
				}
			}
			return Pair.Key.ToString();
		}();
		Lines.Add(FString::Printf(TEXT("  %s: avg %.2f / roll"), *DisplayName, static_cast<float>(Pair.Value) / RollCount));
	}

	UpdateStatusText(FText::FromString(FString::Join(Lines, TEXT("\n"))), FLinearColor(0.9f, 0.9f, 0.9f, 1.0f));
}

void UOnsetItemEditorWidget::UpdateStatusText(const FText& Text, const FLinearColor& Color)
{
	if (StatusText)
	{
		StatusText->SetText(Text);
		StatusText->SetColorAndOpacity(FSlateColor(Color));
	}
}