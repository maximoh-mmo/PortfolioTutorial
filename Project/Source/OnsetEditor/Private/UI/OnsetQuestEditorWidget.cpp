// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/OnsetQuestEditorWidget.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Combat/OnsetEquipmentLibrary.h"
#include "Components/Button.h"
#include "Components/DetailsView.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Data/OnsetDialogueLibrary.h"
#include "Data/OnsetQuestLibrary.h"
#include "Data/OnsetRankTypes.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "Misc/ConfigCacheIni.h"
#include "PackageTools.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "Styling/StyleDefaults.h"
#include "UI/OnsetAbilityRowButton.h"
#include "UI/OnsetNotifyDetailsView.h"
#include "UObject/Package.h"

namespace OnsetQuestEditorWidgetConstants
{
	constexpr float ListEntryHeight = 24.0f;
	constexpr float IdWidth = 160.0f;
	constexpr float NameWidth = 180.0f;
	constexpr float ExtraCol1Width = 80.0f;
	constexpr float ExtraCol2Width = 80.0f;
}

static FName QuestNextWidgetName(const FString& Base)
{
	static int32 WidgetCounter = 0;
	return FName(*FString::Printf(TEXT("%s_%d"), *Base, ++WidgetCounter));
}

static FButtonStyle MakeQuestFlatStyle()
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
	Style.SetNormalPadding(FMargin(6.0f, 2.0f));
	Style.SetPressedPadding(FMargin(6.0f, 3.0f));
	return Style;
}

static FButtonStyle MakeQuestRowStyle(bool bSelected)
{
	FButtonStyle Style = MakeQuestFlatStyle();
	Style.SetNormalPadding(FMargin(0.0f, 1.0f));
	Style.SetPressedPadding(FMargin(0.0f, 2.0f));
	if (bSelected)
	{
		FSlateBrush Selected = *FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
		Selected.TintColor = FSlateColor(FLinearColor(0.15f, 0.40f, 0.85f, 0.38f));
		FSlateBrush SelectedHover = Selected;
		SelectedHover.TintColor = FSlateColor(FLinearColor(0.20f, 0.48f, 0.92f, 0.48f));
		Style.SetNormal(Selected);
		Style.SetHovered(SelectedHover);
		Style.SetPressed(SelectedHover);
	}
	return Style;
}

static USizeBox* MakeQuestCell(UWidgetTree* WidgetTree, const FString& BaseName, float Width, const FText& Text, bool bHeader, ETextJustify::Type Justification)
{
	USizeBox* CellBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), QuestNextWidgetName(BaseName + TEXT("_Cell")));
	CellBox->SetWidthOverride(Width);

	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), QuestNextWidgetName(BaseName + TEXT("_Label")));
	TextBlock->SetText(Text);
	TextBlock->SetJustification(Justification);
	TextBlock->SetFont(bHeader ? FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 10) : FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10));
	if (bHeader)
	{
		TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.70f, 0.70f, 0.70f, 1.0f)));
	}

	CellBox->AddChild(TextBlock);
	return CellBox;
}

static UButton* MakeQuestTextButton(UWidgetTree* WidgetTree, const FText& Label, const FString& BaseName)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), QuestNextWidgetName(BaseName + TEXT("_Button")));
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), QuestNextWidgetName(BaseName + TEXT("_Label")));
	Text->SetText(Label);
	Text->SetJustification(ETextJustify::Center);
	Text->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10));
	Button->SetContent(Text);
	Button->SetStyle(MakeQuestFlatStyle());
	return Button;
}

void UOnsetQuestEditorWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildUI();
	RefreshFromTable();
}

void UOnsetQuestEditorWidget::NativeDestruct()
{
	PersistOnClose();
	Super::NativeDestruct();
}

void UOnsetQuestEditorWidget::OpenEditor()
{
	EnsureAllTablesSeededAndSaved();
	BuildUI();
	RefreshFromTable();
}

void UOnsetQuestEditorWidget::BuildUI()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootBox"));

	// --- Top: Mode Tabs ---
	UHorizontalBox* ModeTabRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ModeTabRow"));
	ModeButtons.Reset();

	const struct FModeButtonConfig
	{
		EOnsetQuestEditorMode Mode;
		FText Label;
	} Configs[] = {
		{ EOnsetQuestEditorMode::Quests, NSLOCTEXT("OnsetQuestEditor", "ModeQuests", "Quests (DT_QuestDefinition)") },
		{ EOnsetQuestEditorMode::NPCs, NSLOCTEXT("OnsetQuestEditor", "ModeNPCs", "NPCs (DT_NPCs)") },
		{ EOnsetQuestEditorMode::Dialogues, NSLOCTEXT("OnsetQuestEditor", "ModeDialogues", "Dialogues (DT_Dialogues)") }
	};

	for (const FModeButtonConfig& Cfg : Configs)
	{
		UOnsetQuestModeButton* Btn = NewObject<UOnsetQuestModeButton>(this);
		Btn->Mode = Cfg.Mode;
		Btn->OnClicked.AddDynamic(Btn, &UOnsetQuestModeButton::HandleClicked);
		Btn->OnModeSelected.AddUObject(this, &UOnsetQuestEditorWidget::OnModeButtonClicked);

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), QuestNextWidgetName(TEXT("ModeBtnLabel")));
		Label->SetText(Cfg.Label);
		Label->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 10));
		Btn->SetContent(Label);
		Btn->SetStyle(MakeQuestFlatStyle());

		ModeTabRow->AddChildToHorizontalBox(Btn)->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));
		ModeButtons.Add(Btn);
	}

	RootBox->AddChildToVerticalBox(ModeTabRow)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));

	// --- Middle: Left list + Right details ---
	UHorizontalBox* ContentRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ContentRow"));

	UVerticalBox* LeftPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftPanel"));

	HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeaderRow"));
	UpdateHeaderCells();
	LeftPanel->AddChildToVerticalBox(HeaderRow);

	UScrollBox* ListScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RowList"));
	ListScroll->SetAlwaysShowScrollbar(true);
	ListScroll->SetAlwaysShowScrollbarTrack(true);
	LeftPanel->AddChildToVerticalBox(ListScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	RowListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RowListBox"));
	ListScroll->AddChild(RowListBox);

	PropertyView = WidgetTree->ConstructWidget<UOnsetNotifyDetailsView>(UOnsetNotifyDetailsView::StaticClass(), TEXT("PropertyView"));
	PropertyView->OnPropertyEdited.AddUObject(this, &UOnsetQuestEditorWidget::HandlePropertyEdited);

	ContentRow->AddChildToHorizontalBox(LeftPanel)->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	ContentRow->AddChildToHorizontalBox(PropertyView)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	RootBox->AddChildToVerticalBox(ContentRow)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// --- Status Text ---
	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	StatusText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10));
	StatusText->SetAutoWrapText(true);
	RootBox->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));

	// --- Bottom: Action Buttons ---
	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonRow"));

	UButton* AddButton = MakeQuestTextButton(WidgetTree, NSLOCTEXT("OnsetQuestEditor", "Add", "+ Add New"), TEXT("Add"));
	AddButton->OnClicked.AddDynamic(this, &UOnsetQuestEditorWidget::AddDefinition);
	ButtonRow->AddChildToHorizontalBox(AddButton)->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));

	UButton* DeleteButton = MakeQuestTextButton(WidgetTree, NSLOCTEXT("OnsetQuestEditor", "Delete", "- Delete"), TEXT("Delete"));
	DeleteButton->OnClicked.AddDynamic(this, &UOnsetQuestEditorWidget::DeleteDefinition);
	ButtonRow->AddChildToHorizontalBox(DeleteButton)->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));

	UButton* ValidateButton = MakeQuestTextButton(WidgetTree, NSLOCTEXT("OnsetQuestEditor", "Validate", "Validate All"), TEXT("Validate"));
	ValidateButton->OnClicked.AddDynamic(this, &UOnsetQuestEditorWidget::ValidateAll);
	ButtonRow->AddChildToHorizontalBox(ValidateButton)->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));

	UButton* SaveButton = MakeQuestTextButton(WidgetTree, NSLOCTEXT("OnsetQuestEditor", "Save", "Save Table"), TEXT("Save"));
	SaveButton->OnClicked.AddDynamic(this, &UOnsetQuestEditorWidget::SaveAll);
	ButtonRow->AddChildToHorizontalBox(SaveButton);

	UVerticalBoxSlot* ButtonRowSlot = RootBox->AddChildToVerticalBox(ButtonRow);
	ButtonRowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	ButtonRowSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
	ButtonRowSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));

	WidgetTree->RootWidget = RootBox;
	UpdateModeButtonHighlight();
}

void UOnsetQuestEditorWidget::UpdateModeButtonHighlight()
{
	for (UOnsetQuestModeButton* ModeButton : ModeButtons)
	{
		if (ModeButton)
		{
			const bool bActive = (ModeButton->Mode == CurrentMode);
			ModeButton->SetStyle(bActive ? MakeQuestRowStyle(true) : MakeQuestFlatStyle());
		}
	}
}

void UOnsetQuestEditorWidget::UpdateHeaderCells()
{
	if (!HeaderRow || !WidgetTree)
	{
		return;
	}

	HeaderRow->ClearChildren();

	switch (CurrentMode)
	{
		case EOnsetQuestEditorMode::Quests:
			HeaderRow->AddChild(MakeQuestCell(WidgetTree, TEXT("HQId"), 160.0f, NSLOCTEXT("OnsetQuestEditor", "ColQuestId", "Quest ID"), true, ETextJustify::Left));
			HeaderRow->AddChild(MakeQuestCell(WidgetTree, TEXT("HQName"), 180.0f, NSLOCTEXT("OnsetQuestEditor", "ColQuestName", "Display Name"), true, ETextJustify::Left));
			HeaderRow->AddChild(MakeQuestCell(WidgetTree, TEXT("HQStages"), 70.0f, NSLOCTEXT("OnsetQuestEditor", "ColStages", "Stages"), true, ETextJustify::Center));
			HeaderRow->AddChild(MakeQuestCell(WidgetTree, TEXT("HQXp"), 70.0f, NSLOCTEXT("OnsetQuestEditor", "ColXp", "XP"), true, ETextJustify::Center));
			break;

		case EOnsetQuestEditorMode::NPCs:
			HeaderRow->AddChild(MakeQuestCell(WidgetTree, TEXT("HNId"), 130.0f, NSLOCTEXT("OnsetQuestEditor", "ColNpcId", "NPC ID"), true, ETextJustify::Left));
			HeaderRow->AddChild(MakeQuestCell(WidgetTree, TEXT("HNName"), 140.0f, NSLOCTEXT("OnsetQuestEditor", "ColNpcName", "Display Name"), true, ETextJustify::Left));
			HeaderRow->AddChild(MakeQuestCell(WidgetTree, TEXT("HNRank"), 140.0f, NSLOCTEXT("OnsetQuestEditor", "ColRank", "Confederate Rank"), true, ETextJustify::Left));
			HeaderRow->AddChild(MakeQuestCell(WidgetTree, TEXT("HNRole"), 140.0f, NSLOCTEXT("OnsetQuestEditor", "ColRole", "Role / Title"), true, ETextJustify::Left));
			HeaderRow->AddChild(MakeQuestCell(WidgetTree, TEXT("HNDial"), 130.0f, NSLOCTEXT("OnsetQuestEditor", "ColEntryRow", "Start Dialogue"), true, ETextJustify::Left));
			break;

		case EOnsetQuestEditorMode::Dialogues:
			HeaderRow->AddChild(MakeQuestCell(WidgetTree, TEXT("HDId"), 160.0f, NSLOCTEXT("OnsetQuestEditor", "ColDialId", "Dialogue Row ID"), true, ETextJustify::Left));
			HeaderRow->AddChild(MakeQuestCell(WidgetTree, TEXT("HDSpeaker"), 130.0f, NSLOCTEXT("OnsetQuestEditor", "ColSpeaker", "Speaker"), true, ETextJustify::Left));
			HeaderRow->AddChild(MakeQuestCell(WidgetTree, TEXT("HDChoices"), 65.0f, NSLOCTEXT("OnsetQuestEditor", "ColChoices", "Choices"), true, ETextJustify::Center));
			HeaderRow->AddChild(MakeQuestCell(WidgetTree, TEXT("HDText"), 220.0f, NSLOCTEXT("OnsetQuestEditor", "ColText", "Dialogue Text"), true, ETextJustify::Left));
			break;
	}
}

void UOnsetQuestEditorWidget::SetMode(EOnsetQuestEditorMode NewMode)
{
	if (NewMode == CurrentMode)
	{
		return;
	}

	WriteBackRow();

	CurrentMode = NewMode;
	SelectedRowName = NAME_None;

	UpdateModeButtonHighlight();
	UpdateHeaderCells();

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

void UOnsetQuestEditorWidget::RefreshFromTable()
{
	RebuildList();
}

void UOnsetQuestEditorWidget::RebuildList()
{
	if (!RowListBox || !WidgetTree)
	{
		return;
	}

	RowListBox->ClearChildren();
	RowButtons.Reset();

	UDataTable* Table = GetTableForMode(CurrentMode);
	if (!Table)
	{
		UpdateStatusText(NSLOCTEXT("OnsetQuestEditor", "MissingTable", "DataTable could not be loaded or created."), FLinearColor(1.0f, 0.4f, 0.4f));
		return;
	}

	EnsureDefaultRowsForTable(CurrentMode, Table);

	TArray<FName> RowNames = Table->GetRowNames();
	RowNames.Sort([](const FName& A, const FName& B) { return A.ToString() < B.ToString(); });

	FName FirstValidRow = NAME_None;

	for (const FName& RowName : RowNames)
	{
		if (RowName.IsNone())
		{
			continue;
		}

		if (FirstValidRow.IsNone())
		{
			FirstValidRow = RowName;
		}

		UOnsetAbilityRowButton* RowBtn = NewObject<UOnsetAbilityRowButton>(this);
		RowBtn->RowName = RowName;
		RowBtn->OnClicked.AddDynamic(RowBtn, &UOnsetAbilityRowButton::HandleClicked);
		RowBtn->OnRowSelected.AddUObject(this, &UOnsetQuestEditorWidget::SelectRow);

		UHorizontalBox* RowHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), QuestNextWidgetName(TEXT("RowHBox")));

		switch (CurrentMode)
		{
			case EOnsetQuestEditorMode::Quests:
			{
				const FOnsetQuestDefinition* Quest = Table->FindRow<FOnsetQuestDefinition>(RowName, nullptr);
				const FText DispName = Quest && !Quest->DisplayName.IsEmpty() ? Quest->DisplayName : FText::FromName(RowName);
				const int32 StageCount = Quest ? Quest->Stages.Num() : 0;
				const int32 Xp = Quest ? Quest->XpReward : 0;

				RowHBox->AddChild(MakeQuestCell(WidgetTree, TEXT("QId"), 160.0f, FText::FromName(RowName), false, ETextJustify::Left));
				RowHBox->AddChild(MakeQuestCell(WidgetTree, TEXT("QName"), 180.0f, DispName, false, ETextJustify::Left));
				RowHBox->AddChild(MakeQuestCell(WidgetTree, TEXT("QStages"), 70.0f, FText::AsNumber(StageCount), false, ETextJustify::Center));
				RowHBox->AddChild(MakeQuestCell(WidgetTree, TEXT("QXp"), 70.0f, FText::AsNumber(Xp), false, ETextJustify::Center));
				break;
			}
			case EOnsetQuestEditorMode::NPCs:
			{
				const FOnsetNPCDefinition* NPC = Table->FindRow<FOnsetNPCDefinition>(RowName, nullptr);
				const FText DispName = NPC && !NPC->DisplayName.IsEmpty() ? NPC->DisplayName : FText::FromName(RowName);
				const FText Role = NPC ? NPC->TitleOrRole : FText::GetEmpty();
				const FText EntryRow = NPC ? FText::FromName(NPC->DefaultDialogueRow) : FText::GetEmpty();
				const FOnsetRankInfo RankInfo = NPC ? UOnsetRankLibrary::GetRankInfo(NPC->MilitaryRank) : FOnsetRankInfo();
				const FText RankText = FText::Format(NSLOCTEXT("OnsetQuestEditor", "RankFmt", "T{0}: {1}"),
					FText::AsNumber(RankInfo.PrecedenceTier), RankInfo.DisplayTitle);

				RowHBox->AddChild(MakeQuestCell(WidgetTree, TEXT("NId"), 130.0f, FText::FromName(RowName), false, ETextJustify::Left));
				RowHBox->AddChild(MakeQuestCell(WidgetTree, TEXT("NName"), 140.0f, DispName, false, ETextJustify::Left));
				RowHBox->AddChild(MakeQuestCell(WidgetTree, TEXT("NRank"), 140.0f, RankText, false, ETextJustify::Left));
				RowHBox->AddChild(MakeQuestCell(WidgetTree, TEXT("NRole"), 140.0f, Role, false, ETextJustify::Left));
				RowHBox->AddChild(MakeQuestCell(WidgetTree, TEXT("NDial"), 130.0f, EntryRow, false, ETextJustify::Left));
				break;
			}
			case EOnsetQuestEditorMode::Dialogues:
			{
				const FOnsetDialogueRow* Dial = Table->FindRow<FOnsetDialogueRow>(RowName, nullptr);
				const FText Speaker = Dial ? Dial->SpeakerName : FText::GetEmpty();
				const int32 ChoiceCount = Dial ? Dial->Choices.Num() : 0;
				FString Preview = Dial ? Dial->DialogueText.ToString() : FString();
				if (Preview.Len() > 32)
				{
					Preview = Preview.Left(32) + TEXT("...");
				}

				RowHBox->AddChild(MakeQuestCell(WidgetTree, TEXT("DId"), 160.0f, FText::FromName(RowName), false, ETextJustify::Left));
				RowHBox->AddChild(MakeQuestCell(WidgetTree, TEXT("DSpeaker"), 130.0f, Speaker, false, ETextJustify::Left));
				RowHBox->AddChild(MakeQuestCell(WidgetTree, TEXT("DChoices"), 65.0f, FText::AsNumber(ChoiceCount), false, ETextJustify::Center));
				RowHBox->AddChild(MakeQuestCell(WidgetTree, TEXT("DText"), 220.0f, FText::FromString(Preview), false, ETextJustify::Left));
				break;
			}
		}

		RowBtn->SetContent(RowHBox);
		RowBtn->SetStyle(MakeQuestRowStyle(RowName == SelectedRowName));

		UVerticalBoxSlot* RowBoxSlot = RowListBox->AddChildToVerticalBox(RowBtn);
		RowBoxSlot->SetPadding(FMargin(0.0f, 1.0f));

		RowButtons.Add(RowName, RowBtn);
	}

	if (!SelectedRowName.IsNone() && RowNames.Contains(SelectedRowName))
	{
		SelectRow(SelectedRowName);
	}
	else if (!FirstValidRow.IsNone())
	{
		SelectRow(FirstValidRow);
	}
	else if (PropertyView)
	{
		PropertyView->SetObject(nullptr);
	}
}

void UOnsetQuestEditorWidget::SelectRow(FName RowName)
{
	if (RowName.IsNone())
	{
		return;
	}

	WriteBackRow();
	SelectedRowName = RowName;

	UDataTable* Table = GetTableForMode(CurrentMode);
	if (!Table)
	{
		return;
	}

	switch (CurrentMode)
	{
		case EOnsetQuestEditorMode::Quests:
		{
			if (const FOnsetQuestDefinition* Row = Table->FindRow<FOnsetQuestDefinition>(RowName, nullptr))
			{
				if (!QuestWrapper)
				{
					QuestWrapper = NewObject<UOnsetQuestRowWrapper>(this);
				}
				QuestWrapper->Row = *Row;
				if (PropertyView) PropertyView->SetObject(QuestWrapper);
			}
			break;
		}
		case EOnsetQuestEditorMode::NPCs:
		{
			if (const FOnsetNPCDefinition* Row = Table->FindRow<FOnsetNPCDefinition>(RowName, nullptr))
			{
				if (!NPCWrapper)
				{
					NPCWrapper = NewObject<UOnsetNPCRowWrapper>(this);
				}
				NPCWrapper->Row = *Row;
				if (PropertyView) PropertyView->SetObject(NPCWrapper);
			}
			break;
		}
		case EOnsetQuestEditorMode::Dialogues:
		{
			if (const FOnsetDialogueRow* Row = Table->FindRow<FOnsetDialogueRow>(RowName, nullptr))
			{
				if (!DialogueWrapper)
				{
					DialogueWrapper = NewObject<UOnsetDialogueRowWrapper>(this);
				}
				DialogueWrapper->Row = *Row;
				if (PropertyView) PropertyView->SetObject(DialogueWrapper);
			}
			break;
		}
	}

	for (const TPair<FName, TObjectPtr<UOnsetAbilityRowButton>>& Pair : RowButtons)
	{
		if (Pair.Value)
		{
			Pair.Value->SetStyle(MakeQuestRowStyle(Pair.Key == SelectedRowName));
		}
	}
}

bool UOnsetQuestEditorWidget::WriteBackRow()
{
	UDataTable* Table = GetTableForMode(CurrentMode);
	if (!Table || SelectedRowName.IsNone())
	{
		return false;
	}

	switch (CurrentMode)
	{
		case EOnsetQuestEditorMode::Quests:
			if (QuestWrapper)
			{
				if (FOnsetQuestDefinition* Row = Table->FindRow<FOnsetQuestDefinition>(SelectedRowName, nullptr))
				{
					*Row = QuestWrapper->Row;
					return true;
				}
			}
			break;
		case EOnsetQuestEditorMode::NPCs:
			if (NPCWrapper)
			{
				if (FOnsetNPCDefinition* Row = Table->FindRow<FOnsetNPCDefinition>(SelectedRowName, nullptr))
				{
					*Row = NPCWrapper->Row;
					return true;
				}
			}
			break;
		case EOnsetQuestEditorMode::Dialogues:
			if (DialogueWrapper)
			{
				if (FOnsetDialogueRow* Row = Table->FindRow<FOnsetDialogueRow>(SelectedRowName, nullptr))
				{
					*Row = DialogueWrapper->Row;
					return true;
				}
			}
			break;
	}

	return false;
}

void UOnsetQuestEditorWidget::HandlePropertyEdited()
{
	WriteBackRow();
	DirtyModes.Add(CurrentMode);

	if (UDataTable* Table = GetTableForMode(CurrentMode))
	{
		Table->GetPackage()->MarkPackageDirty();
	}

	UpdateStatusText(FText::Format(NSLOCTEXT("OnsetQuestEditor", "RowDirty", "Edited '{0}' (unsaved changes)"), FText::FromName(SelectedRowName)), FLinearColor(0.9f, 0.7f, 0.2f));
}

void UOnsetQuestEditorWidget::AddDefinition()
{
	UDataTable* Table = GetTableForMode(CurrentMode);
	if (!Table)
	{
		return;
	}

	FString Prefix = TEXT("NewQuest");
	if (CurrentMode == EOnsetQuestEditorMode::NPCs) Prefix = TEXT("NewNPC");
	else if (CurrentMode == EOnsetQuestEditorMode::Dialogues) Prefix = TEXT("NewDialogue");

	FName NewRowName = FName(*Prefix);
	int32 Counter = 1;
	while (Table->GetRowNames().Contains(NewRowName))
	{
		NewRowName = FName(*FString::Printf(TEXT("%s_%d"), *Prefix, Counter++));
	}

	switch (CurrentMode)
	{
		case EOnsetQuestEditorMode::Quests:
		{
			FOnsetQuestDefinition NewQuest;
			NewQuest.DisplayName = FText::FromString(TEXT("New Mission"));
			NewQuest.Description = FText::FromString(TEXT("Mission briefing description."));
			NewQuest.XpReward = 50;

			FOnsetQuestStageDefinition Stage1;
			Stage1.Description = FText::FromString(TEXT("Stage 1"));
			FOnsetQuestObjectiveDefinition Obj;
			Obj.Type = EOnsetQuestObjectiveType::Kill;
			Obj.Count = 5;
			Obj.Description = FText::FromString(TEXT("Objective 1"));
			Stage1.Objectives.Add(Obj);
			NewQuest.Stages.Add(Stage1);

			Table->AddRow(NewRowName, NewQuest);
			break;
		}
		case EOnsetQuestEditorMode::NPCs:
		{
			FOnsetNPCDefinition NewNPC;
			NewNPC.DisplayName = FText::FromString(TEXT("New Officer"));
			NewNPC.TitleOrRole = FText::FromString(TEXT("Operative"));
			NewNPC.InteractionPrompt = FText::FromString(TEXT("Talk"));
			Table->AddRow(NewRowName, NewNPC);
			break;
		}
		case EOnsetQuestEditorMode::Dialogues:
		{
			FOnsetDialogueRow NewDialogue;
			NewDialogue.SpeakerName = FText::FromString(TEXT("Officer"));
			NewDialogue.DialogueText = FText::FromString(TEXT("State your purpose, Cadet."));
			FOnsetDialogueChoice Choice;
			Choice.ResponseText = FText::FromString(TEXT("Understood."));
			NewDialogue.Choices.Add(Choice);
			Table->AddRow(NewRowName, NewDialogue);
			break;
		}
	}

	Table->GetPackage()->MarkPackageDirty();
	DirtyModes.Add(CurrentMode);

	RefreshFromTable();
	SelectRow(NewRowName);

	UpdateStatusText(FText::Format(NSLOCTEXT("OnsetQuestEditor", "RowAdded", "Created new entry '{0}'"), FText::FromName(NewRowName)), FLinearColor(0.2f, 0.9f, 0.4f));
}

void UOnsetQuestEditorWidget::DeleteDefinition()
{
	UDataTable* Table = GetTableForMode(CurrentMode);
	if (!Table || SelectedRowName.IsNone())
	{
		return;
	}

	const FName Deleted = SelectedRowName;
	Table->RemoveRow(SelectedRowName);
	SelectedRowName = NAME_None;

	if (PropertyView)
	{
		PropertyView->SetObject(nullptr);
	}

	Table->GetPackage()->MarkPackageDirty();
	DirtyModes.Add(CurrentMode);

	RefreshFromTable();
	UpdateStatusText(FText::Format(NSLOCTEXT("OnsetQuestEditor", "RowDeleted", "Deleted entry '{0}'"), FText::FromName(Deleted)), FLinearColor(1.0f, 0.4f, 0.4f));
}

void UOnsetQuestEditorWidget::ValidateAll()
{
	WriteBackRow();

	UDataTable* Table = GetTableForMode(CurrentMode);
	if (!Table)
	{
		return;
	}

	const FString Errors = ValidateTable(CurrentMode, Table);
	if (Errors.IsEmpty())
	{
		UpdateStatusText(FText::Format(NSLOCTEXT("OnsetQuestEditor", "ValidSuccess", "All {0} entries in table '{1}' validated successfully (0 errors)."),
			FText::AsNumber(Table->GetRowNames().Num()), FText::FromString(Table->GetName())),
			FLinearColor(0.2f, 0.9f, 0.3f));
	}
	else
	{
		UpdateStatusText(FText::FromString(Errors), FLinearColor(1.0f, 0.35f, 0.35f));
	}
}

void UOnsetQuestEditorWidget::SaveAll()
{
	WriteBackRow();

	TArray<UObject*> ToSave;
	for (const EOnsetQuestEditorMode Mode : { EOnsetQuestEditorMode::Quests, EOnsetQuestEditorMode::NPCs, EOnsetQuestEditorMode::Dialogues })
	{
		if (UDataTable* Table = GetTableForMode(Mode))
		{
			ToSave.AddUnique(Table);
		}
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
		DirtyModes.Reset();
		UpdateStatusText(NSLOCTEXT("OnsetQuestEditor", "SavedSuccess", "All Quest & Dialogue DataTables saved to disk successfully."), FLinearColor(0.2f, 0.9f, 0.4f));
	}
}

void UOnsetQuestEditorWidget::PersistOnClose()
{
	if (DirtyModes.Num() == 0)
	{
		return;
	}

	WriteBackRow();

	TArray<UObject*> ToSave;
	for (const EOnsetQuestEditorMode Mode : DirtyModes)
	{
		if (UDataTable* Table = GetTableForMode(Mode))
		{
			ToSave.AddUnique(Table);
		}
	}

	if (ToSave.Num() > 0)
	{
		for (UObject* Obj : ToSave)
		{
			if (Obj) Obj->GetPackage()->MarkPackageDirty();
		}
		UPackageTools::SavePackagesForObjects(ToSave);
	}

	DirtyModes.Reset();
}

void UOnsetQuestEditorWidget::EnsureAllTablesSeededAndSaved()
{
	const EOnsetQuestEditorMode Modes[] = {
		EOnsetQuestEditorMode::Quests,
		EOnsetQuestEditorMode::NPCs,
		EOnsetQuestEditorMode::Dialogues
	};

	TArray<UObject*> TablesToSave;

	for (EOnsetQuestEditorMode Mode : Modes)
	{
		UDataTable* Table = GetTableForMode(Mode);
		if (Table)
		{
			const bool bModified = EnsureDefaultRowsForTable(Mode, Table);
			if (bModified || Table->GetPackage()->IsDirty())
			{
				TablesToSave.AddUnique(Table);
			}
		}
	}

	if (TablesToSave.Num() > 0)
	{
		for (UObject* Obj : TablesToSave)
		{
			if (Obj && Obj->GetPackage())
			{
				Obj->GetPackage()->MarkPackageDirty();
			}
		}
		UPackageTools::SavePackagesForObjects(TablesToSave);
	}
}

UDataTable* UOnsetQuestEditorWidget::GetTableForMode(EOnsetQuestEditorMode Mode)
{
	FString TablePath;
	UScriptStruct* Struct = nullptr;

	switch (Mode)
	{
		case EOnsetQuestEditorMode::Quests:
			TablePath = UOnsetQuestLibrary::GetTablePath();
			Struct = FOnsetQuestDefinition::StaticStruct();
			break;
		case EOnsetQuestEditorMode::NPCs:
			TablePath = UOnsetDialogueLibrary::GetNPCTablePath();
			Struct = FOnsetNPCDefinition::StaticStruct();
			break;
		case EOnsetQuestEditorMode::Dialogues:
			TablePath = UOnsetDialogueLibrary::GetDialogueTablePath();
			Struct = FOnsetDialogueRow::StaticStruct();
			break;
	}

	if (TablePath.IsEmpty() || !Struct)
	{
		return nullptr;
	}

	if (UDataTable* Existing = Cast<UDataTable>(StaticFindObject(UDataTable::StaticClass(), nullptr, *TablePath)))
	{
		if (!Existing->RowStruct)
		{
			Existing->RowStruct = Struct;
		}
		return Existing;
	}

	const FString PkgName = FPackageName::ObjectPathToPackageName(TablePath);
	if (FPackageName::DoesPackageExist(PkgName))
	{
		if (UDataTable* Existing = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *TablePath)))
		{
			if (!Existing->RowStruct)
			{
				Existing->RowStruct = Struct;
			}
			return Existing;
		}
	}

	// Create the package and asset if it does not exist yet
	FString PackageName;
	FString AssetName;
	if (!TablePath.Split(TEXT("."), &PackageName, &AssetName))
	{
		PackageName = TablePath;
		AssetName = FPackageName::GetShortName(TablePath);
	}

	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		return nullptr;
	}

	if (UDataTable* ExistingInPkg = FindObject<UDataTable>(Package, *AssetName))
	{
		if (!ExistingInPkg->RowStruct)
		{
			ExistingInPkg->RowStruct = Struct;
		}
		return ExistingInPkg;
	}

	UDataTable* NewTable = NewObject<UDataTable>(Package, FName(*AssetName), RF_Public | RF_Standalone);
	if (NewTable)
	{
		NewTable->RowStruct = Struct;
		FAssetRegistryModule::AssetCreated(NewTable);
		Package->MarkPackageDirty();
	}

	return NewTable;
}

bool UOnsetQuestEditorWidget::EnsureDefaultRowsForTable(EOnsetQuestEditorMode Mode, UDataTable* Table)
{
	if (!Table)
	{
		return false;
	}

	TArray<FName> ExistingRows = Table->GetRowNames();
	// If only placeholder "NewRow" or empty, remove placeholder and populate with authored Academy defaults!
	const bool bNeedsSeeding = ExistingRows.Num() == 0 || (ExistingRows.Num() == 1 && ExistingRows[0] == FName(TEXT("NewRow")));
	if (!bNeedsSeeding)
	{
		return false;
	}

	if (ExistingRows.Contains(FName(TEXT("NewRow"))))
	{
		Table->RemoveRow(FName(TEXT("NewRow")));
	}

	switch (Mode)
	{
		case EOnsetQuestEditorMode::Quests:
		{
			// FirstBlood
			{
				FOnsetQuestDefinition Quest;
				Quest.DisplayName = FText::FromString(TEXT("First Blood"));
				Quest.Description = FText::FromString(TEXT("Neutralize hostiles outside the perimeter wall to verify weapon systems."));
				Quest.XpReward = 25;
				FOnsetQuestStageDefinition Stage1;
				Stage1.Description = FText::FromString(TEXT("Neutralize Grunts on the perimeter."));
				FOnsetQuestObjectiveDefinition Obj;
				Obj.Type = EOnsetQuestObjectiveType::Kill;
				Obj.Target = FName(TEXT("Grunt"));
				Obj.Count = 3;
				Obj.Description = FText::FromString(TEXT("Neutralize Grunts (3)"));
				Stage1.Objectives.Add(Obj);
				Quest.Stages.Add(Stage1);
				Table->AddRow(FName(TEXT("FirstBlood")), Quest);
			}
			// Q1_CadetInduction
			{
				FOnsetQuestDefinition Quest;
				Quest.DisplayName = FText::FromString(TEXT("Cadet Induction"));
				Quest.Description = FText::FromString(TEXT("Cycle the security bulkhead, calibrate combat telemetry with Instructor Kael, and report to Commander Vane."));
				Quest.XpReward = 30;
				FOnsetQuestStageDefinition Stage1;
				Stage1.Description = FText::FromString(TEXT("Complete Induction Orientation in the Academy."));
				FOnsetQuestObjectiveDefinition ObjBulkhead;
				ObjBulkhead.Type = EOnsetQuestObjectiveType::Interact;
				ObjBulkhead.Target = FName(TEXT("Door_Induction"));
				ObjBulkhead.Count = 1;
				ObjBulkhead.Description = FText::FromString(TEXT("Cycle Induction Security Bulkhead"));
				Stage1.Objectives.Add(ObjBulkhead);
				FOnsetQuestObjectiveDefinition ObjKael;
				ObjKael.Type = EOnsetQuestObjectiveType::Interact;
				ObjKael.Target = FName(TEXT("InstructorKael"));
				ObjKael.Count = 1;
				ObjKael.Description = FText::FromString(TEXT("Calibrate Combat Telemetry with Instructor Kael"));
				Stage1.Objectives.Add(ObjKael);
				Quest.Stages.Add(Stage1);
				FOnsetQuestStageDefinition Stage2;
				Stage2.Description = FText::FromString(TEXT("Report to Commander Vane at the South Gate."));
				FOnsetQuestObjectiveDefinition ObjVane;
				ObjVane.Type = EOnsetQuestObjectiveType::Interact;
				ObjVane.Target = FName(TEXT("ElderVane"));
				ObjVane.Count = 1;
				ObjVane.Description = FText::FromString(TEXT("Report to Commander Vane"));
				Stage2.Objectives.Add(ObjVane);
				Quest.Stages.Add(Stage2);
				Table->AddRow(FName(TEXT("Q1_CadetInduction")), Quest);
			}
			// Q2_BioDomeContainment
			{
				FOnsetQuestDefinition Quest;
				Quest.DisplayName = FText::FromString(TEXT("Bio-Dome Containment"));
				Quest.Description = FText::FromString(TEXT("Clear biological mutations in the Sector 1 Bio-Dome Terraces and scan the breached telemetry monitor."));
				Quest.XpReward = 45;
				FOnsetQuestStageDefinition Stage1;
				Stage1.Description = FText::FromString(TEXT("Neutralize Bio-Stalkers in Sector 1."));
				FOnsetQuestObjectiveDefinition ObjKill;
				ObjKill.Type = EOnsetQuestObjectiveType::Kill;
				ObjKill.Target = FName(TEXT("Academy_Lv1_BioStalker"));
				ObjKill.Count = 8;
				ObjKill.Description = FText::FromString(TEXT("Neutralize Bio-Stalkers (8)"));
				Stage1.Objectives.Add(ObjKill);
				Quest.Stages.Add(Stage1);
				FOnsetQuestStageDefinition Stage2;
				Stage2.Description = FText::FromString(TEXT("Download monitor telemetry and rendezvous with Operative Tam."));
				FOnsetQuestObjectiveDefinition ObjScan;
				ObjScan.Type = EOnsetQuestObjectiveType::Interact;
				ObjScan.Target = FName(TEXT("Trigger_BioMonitor"));
				ObjScan.Count = 1;
				ObjScan.Description = FText::FromString(TEXT("Download Telemetry from Breached Bio-Monitor"));
				Stage2.Objectives.Add(ObjScan);
				FOnsetQuestObjectiveDefinition ObjTam;
				ObjTam.Type = EOnsetQuestObjectiveType::Interact;
				ObjTam.Target = FName(TEXT("OperativeTam"));
				ObjTam.Count = 1;
				ObjTam.Description = FText::FromString(TEXT("Rendezvous with Operative Tam"));
				Stage2.Objectives.Add(ObjTam);
				Quest.Stages.Add(Stage2);
				Table->AddRow(FName(TEXT("Q2_BioDomeContainment")), Quest);
			}
			// Q3_CoolantMalfunction
			{
				FOnsetQuestDefinition Quest;
				Quest.DisplayName = FText::FromString(TEXT("Coolant Malfunction"));
				Quest.Description = FText::FromString(TEXT("Cycle the turbine floodgate console and eliminate corrosive rogue drones in Sector 2."));
				Quest.XpReward = 55;
				FOnsetQuestStageDefinition Stage1;
				Stage1.Description = FText::FromString(TEXT("Drain the reservoir floodgate."));
				FOnsetQuestObjectiveDefinition ObjConsole;
				ObjConsole.Type = EOnsetQuestObjectiveType::Interact;
				ObjConsole.Target = FName(TEXT("Trigger_CoolantConsole"));
				ObjConsole.Count = 1;
				ObjConsole.Description = FText::FromString(TEXT("Cycle Coolant Flow Console"));
				Stage1.Objectives.Add(ObjConsole);
				Quest.Stages.Add(Stage1);
				FOnsetQuestStageDefinition Stage2;
				Stage2.Description = FText::FromString(TEXT("Destroy rogue drones and reach Recon Officer Mira."));
				FOnsetQuestObjectiveDefinition ObjKillDrones;
				ObjKillDrones.Type = EOnsetQuestObjectiveType::Kill;
				ObjKillDrones.Target = FName(TEXT("Academy_Lv2_RogueDrone"));
				ObjKillDrones.Count = 12;
				ObjKillDrones.Description = FText::FromString(TEXT("Destroy Rogue Drones (12)"));
				Stage2.Objectives.Add(ObjKillDrones);
				FOnsetQuestObjectiveDefinition ObjMira;
				ObjMira.Type = EOnsetQuestObjectiveType::Interact;
				ObjMira.Target = FName(TEXT("ReconMira"));
				ObjMira.Count = 1;
				ObjMira.Description = FText::FromString(TEXT("Report to Recon Officer Mira"));
				Stage2.Objectives.Add(ObjMira);
				Quest.Stages.Add(Stage2);
				Table->AddRow(FName(TEXT("Q3_CoolantMalfunction")), Quest);
			}
			// Q4_SignalInterference
			{
				FOnsetQuestDefinition Quest;
				Quest.DisplayName = FText::FromString(TEXT("Signal Interference"));
				Quest.Description = FText::FromString(TEXT("Reclaim the comms relay dish, restore the East Service Airlock, and stabilize the geothermal excavation pylons."));
				Quest.XpReward = 70;
				FOnsetQuestStageDefinition Stage1;
				Stage1.Description = FText::FromString(TEXT("Eliminate Outcast scavengers and engage the East Airlock shortcut."));
				FOnsetQuestObjectiveDefinition ObjKillRaiders;
				ObjKillRaiders.Type = EOnsetQuestObjectiveType::Kill;
				ObjKillRaiders.Target = FName(TEXT("Academy_Lv3_OutcastRaider"));
				ObjKillRaiders.Count = 12;
				ObjKillRaiders.Description = FText::FromString(TEXT("Eliminate Outcast Raiders (12)"));
				Stage1.Objectives.Add(ObjKillRaiders);
				FOnsetQuestObjectiveDefinition ObjShortcut;
				ObjShortcut.Type = EOnsetQuestObjectiveType::Interact;
				ObjShortcut.Target = FName(TEXT("Trigger_EastShortcut"));
				ObjShortcut.Count = 1;
				ObjShortcut.Description = FText::FromString(TEXT("Activate East Airlock Override Console"));
				Stage1.Objectives.Add(ObjShortcut);
				Quest.Stages.Add(Stage1);
				FOnsetQuestStageDefinition Stage2;
				Stage2.Description = FText::FromString(TEXT("Dismantle excavator mechs in the geothermal bore pit."));
				FOnsetQuestObjectiveDefinition ObjKillMechs;
				ObjKillMechs.Type = EOnsetQuestObjectiveType::Kill;
				ObjKillMechs.Target = FName(TEXT("Academy_Lv4_ExcavatorUnit"));
				ObjKillMechs.Count = 12;
				ObjKillMechs.Description = FText::FromString(TEXT("Dismantle Excavator Units (12)"));
				Stage2.Objectives.Add(ObjKillMechs);
				FOnsetQuestObjectiveDefinition ObjPylon;
				ObjPylon.Type = EOnsetQuestObjectiveType::Interact;
				ObjPylon.Target = FName(TEXT("Trigger_PsionicPylon"));
				ObjPylon.Count = 1;
				ObjPylon.Description = FText::FromString(TEXT("Cycle Psionic Stabilization Pylon"));
				Stage2.Objectives.Add(ObjPylon);
				Quest.Stages.Add(Stage2);
				Table->AddRow(FName(TEXT("Q4_SignalInterference")), Quest);
			}
			// Q5_ThePatternRevealed
			{
				FOnsetQuestDefinition Quest;
				Quest.DisplayName = FText::FromString(TEXT("The Pattern Revealed"));
				Quest.Description = FText::FromString(TEXT("Unlock the West Containment Bulkhead, purge the Aberration Breach, and inspect the Frontier Slipstream Gate."));
				Quest.XpReward = 90;
				FOnsetQuestStageDefinition Stage1;
				Stage1.Description = FText::FromString(TEXT("Contain the Aberration Breach in Sector 5."));
				FOnsetQuestObjectiveDefinition ObjWestShortcut;
				ObjWestShortcut.Type = EOnsetQuestObjectiveType::Interact;
				ObjWestShortcut.Target = FName(TEXT("Trigger_WestShortcut"));
				ObjWestShortcut.Count = 1;
				ObjWestShortcut.Description = FText::FromString(TEXT("Engage West Bulkhead Override Console"));
				Stage1.Objectives.Add(ObjWestShortcut);
				FOnsetQuestObjectiveDefinition ObjPurge;
				ObjPurge.Type = EOnsetQuestObjectiveType::Kill;
				ObjPurge.Target = FName(TEXT("Academy_Lv5_RiftAberration"));
				ObjPurge.Count = 15;
				ObjPurge.Description = FText::FromString(TEXT("Purge Rift Aberrations (15)"));
				Stage1.Objectives.Add(ObjPurge);
				Quest.Stages.Add(Stage1);
				Table->AddRow(FName(TEXT("Q5_ThePatternRevealed")), Quest);
			}
			break;
		}
		case EOnsetQuestEditorMode::NPCs:
		{
			auto AddNPC = [&](FName RowName, const FString& Name, const FString& Role, const FString& Prompt, FName DefaultRow,
				EOnsetRankId Rank = EOnsetRankId::Civilian,
				EOnsetHierarchyBranch Branch = EOnsetHierarchyBranch::Governance,
				EOnsetSpecialism Spec = EOnsetSpecialism::None,
				EOnsetProtocolStyle Style = EOnsetProtocolStyle::CivilianGreeting,
				const FString& AppearancePath = TEXT(""))
			{
				FOnsetNPCDefinition Def;
				Def.DisplayName = FText::FromString(Name);
				Def.TitleOrRole = FText::FromString(Role);
				Def.InteractionPrompt = FText::FromString(Prompt);
				Def.DefaultDialogueRow = DefaultRow;
				Def.MilitaryRank = Rank;
				Def.HierarchyBranch = Branch;
				Def.Specialism = Spec;
				Def.ProtocolStyle = Style;
				if (!AppearancePath.IsEmpty())
				{
					Def.AuthoredAppearance = TSoftObjectPtr<UOnsetAuthoredAppearance>(FSoftObjectPath(AppearancePath));
				}
				Table->AddRow(RowName, Def);
			};

			AddNPC(FName(TEXT("DirectorSolen")), TEXT("Director Solen"), TEXT("Academy Provost"), TEXT("Speak"), FName(TEXT("Solen_Intro")),
				EOnsetRankId::DirectorateProvost, EOnsetHierarchyBranch::Substrate, EOnsetSpecialism::SubstratePhysics, EOnsetProtocolStyle::ResonantNod);

			AddNPC(FName(TEXT("ElderVane")), TEXT("Commander Vane"), TEXT("Vanguard Operations Officer"), TEXT("Briefing"), FName(TEXT("ElderVane_Intro")),
				EOnsetRankId::LegatusCommander, EOnsetHierarchyBranch::Military, EOnsetSpecialism::ShockAssault, EOnsetProtocolStyle::MilitarySalute,
				TEXT("/Game/OnsetCharacterCreation/Appearances/App_AcademyCadreInstructor.App_AcademyCadreInstructor"));

			AddNPC(FName(TEXT("InstructorKael")), TEXT("Instructor Kael"), TEXT("Combat Master (House Vanguard)"), TEXT("Spar / Train"), FName(TEXT("Kael_Intro")),
				EOnsetRankId::SeniorOfficer, EOnsetHierarchyBranch::Military, EOnsetSpecialism::BastionDefense, EOnsetProtocolStyle::MilitarySalute,
				TEXT("/Game/OnsetCharacterCreation/Appearances/App_AcademyCadreInstructor.App_AcademyCadreInstructor"));

			AddNPC(FName(TEXT("QuartermasterBren")), TEXT("Quartermaster Bren"), TEXT("Logistics Officer (House Aegis)"), TEXT("Requisition"), FName(TEXT("Bren_Intro")),
				EOnsetRankId::LogisticsOfficer, EOnsetHierarchyBranch::Logistics, EOnsetSpecialism::OrdnanceLogistics, EOnsetProtocolStyle::MilitarySalute,
				TEXT("/Game/OnsetCharacterCreation/Appearances/App_AegisQuartermaster.App_AegisQuartermaster"));

			AddNPC(FName(TEXT("AnalystLyra")), TEXT("Analyst Lyra"), TEXT("Substrate Researcher (House Meridian)"), TEXT("Consult Intel"), FName(TEXT("Lyra_Intro")),
				EOnsetRankId::SeniorAnalyst, EOnsetHierarchyBranch::Substrate, EOnsetSpecialism::SubstratePhysics, EOnsetProtocolStyle::ResonantNod,
				TEXT("/Game/OnsetCharacterCreation/Appearances/App_MeridianAnalyst.App_MeridianAnalyst"));

			AddNPC(FName(TEXT("OperativeTam")), TEXT("Operative Tam"), TEXT("Field Scout (House Nova)"), TEXT("Status Report"), FName(TEXT("Tam_Intro")),
				EOnsetRankId::SeniorScout, EOnsetHierarchyBranch::SpecialOps, EOnsetSpecialism::ReconSniper, EOnsetProtocolStyle::MilitarySalute);

			AddNPC(FName(TEXT("ReconMira")), TEXT("Recon Officer Mira"), TEXT("Reconnaissance Lead"), TEXT("Rendezvous"), FName(TEXT("Mira_Intro")),
				EOnsetRankId::FieldOperative, EOnsetHierarchyBranch::SpecialOps, EOnsetSpecialism::SignalsIntel, EOnsetProtocolStyle::MilitarySalute);

			AddNPC(FName(TEXT("GovernorThorne")), TEXT("Governor Thorne"), TEXT("Planetary Governor (Sector Consul)"), TEXT("Audience"), FName(TEXT("Thorne_Intro")),
				EOnsetRankId::PlanetaryGovernor, EOnsetHierarchyBranch::Governance, EOnsetSpecialism::CivilAdministration, EOnsetProtocolStyle::DiplomaticBow);

			AddNPC(FName(TEXT("SenatorVass")), TEXT("Senator Vass"), TEXT("Confederate High Council Envoy"), TEXT("Confer"), FName(TEXT("Vass_Intro")),
				EOnsetRankId::SectorSenator, EOnsetHierarchyBranch::Governance, EOnsetSpecialism::DiplomaticCorps, EOnsetProtocolStyle::DiplomaticBow);

			break;
		}
		case EOnsetQuestEditorMode::Dialogues:
		{
			auto AddDial = [&](FName RowName, const FString& Speaker, const FString& Text, const TArray<FOnsetDialogueChoice>& Choices)
			{
				FOnsetDialogueRow Row;
				Row.SpeakerName = FText::FromString(Speaker);
				Row.DialogueText = FText::FromString(Text);
				Row.Choices = Choices;
				Table->AddRow(RowName, Row);
			};

			// Solen
			{
				FOnsetDialogueChoice C1; C1.ResponseText = FText::FromString(TEXT("Cadet reporting for induction, Director. [Accept: Cadet Induction]")); C1.GrantQuestOnSelect = FName(TEXT("Q1_CadetInduction")); C1.NextDialogueRow = FName(TEXT("Solen_Accepted"));
				FOnsetDialogueChoice C2; C2.ResponseText = FText::FromString(TEXT("What is the mission of this facility?")); C2.NextDialogueRow = FName(TEXT("Solen_Lore"));
				FOnsetDialogueChoice C3; C3.ResponseText = FText::FromString(TEXT("Standing by."));
				AddDial(FName(TEXT("Solen_Intro")), TEXT("Director Solen"), TEXT("Welcome to the Academy Concourse, Cadet. Our forward outposts report anomalous energy surges along the perimeter."), { C1, C2, C3 });

				FOnsetDialogueChoice CBack; CBack.ResponseText = FText::FromString(TEXT("Ready for assignment.")); CBack.NextDialogueRow = FName(TEXT("Solen_Intro"));
				AddDial(FName(TEXT("Solen_Lore")), TEXT("Director Solen"), TEXT("The public sees isolated skirmishes. Leadership sees a pattern."), { CBack, C3 });

				FOnsetDialogueChoice CClose; CClose.ResponseText = FText::FromString(TEXT("On my way."));
				AddDial(FName(TEXT("Solen_Accepted")), TEXT("Director Solen"), TEXT("Cycle the Induction Security Bulkhead, verify weapon calibration with Instructor Kael, then report to Commander Vane."), { CClose });
				AddDial(FName(TEXT("Solen_Completed")), TEXT("Director Solen"), TEXT("Your induction telemetry is synced to Command network. Serve with honour."), { C3 });
			}

			// Vane
			{
				FOnsetDialogueChoice CAcceptQ2; CAcceptQ2.ResponseText = FText::FromString(TEXT("Ready for perimeter sortie, Commander. [Accept: Bio-Dome Containment]")); CAcceptQ2.GrantQuestOnSelect = FName(TEXT("Q2_BioDomeContainment")); CAcceptQ2.NextDialogueRow = FName(TEXT("Vane_Q2Accepted"));
				FOnsetDialogueChoice CFirstBlood; CFirstBlood.ResponseText = FText::FromString(TEXT("Reporting for duty, Commander. [Accept Mission: FirstBlood]")); CFirstBlood.GrantQuestOnSelect = FName(TEXT("FirstBlood")); CFirstBlood.NextDialogueRow = FName(TEXT("ElderVane_Accepted"));
				FOnsetDialogueChoice CLore; CLore.ResponseText = FText::FromString(TEXT("What is the tactical situation?")); CLore.NextDialogueRow = FName(TEXT("ElderVane_Lore"));
				FOnsetDialogueChoice CLeave; CLeave.ResponseText = FText::FromString(TEXT("Dismissed."));
				AddDial(FName(TEXT("ElderVane_Intro")), TEXT("Commander Vane"), TEXT("Cadet, telemetry shows rising aberration signatures outside the South Blast Gate."), { CAcceptQ2, CFirstBlood, CLore, CLeave });
				AddDial(FName(TEXT("ElderVane_Lore")), TEXT("Commander Vane"), TEXT("Security bulkheads seal the inner sectors. Look for override consoles to unlock transit."), { CLore, CLeave });
				AddDial(FName(TEXT("ElderVane_Accepted")), TEXT("Commander Vane"), TEXT("Keep your stillsuit telemetry active and neutralize hostiles."), { CLeave });
				AddDial(FName(TEXT("Vane_Q2Accepted")), TEXT("Commander Vane"), TEXT("Step through the South Blast Gate into Sector 1. Cull the Bio-Stalkers."), { CLeave });
				AddDial(FName(TEXT("ElderVane_Completed")), TEXT("Commander Vane"), TEXT("Clean execution, Operative. Service record updated."), { CLeave });
			}

			// Lyra
			{
				FOnsetDialogueChoice C1; C1.ResponseText = FText::FromString(TEXT("Explain the elemental affinities of the perimeter threats.")); C1.NextDialogueRow = FName(TEXT("Lyra_Affinities"));
				FOnsetDialogueChoice C2; C2.ResponseText = FText::FromString(TEXT("Thank you, Analyst."));
				AddDial(FName(TEXT("Lyra_Intro")), TEXT("Analyst Lyra"), TEXT("The central holo-projector tracks anomalous field signatures across all five sectors."), { C1, C2 });
				AddDial(FName(TEXT("Lyra_Affinities")), TEXT("Analyst Lyra"), TEXT("Bio-Stalkers are Physical. Drones leak Poison. Outcasts use Ice. Excavators channel Lightning. Rift Aberrations burn Fire."), { C2 });
			}

			// Kael
			{
				FOnsetDialogueChoice C1; C1.ResponseText = FText::FromString(TEXT("Calibrating weapon and shield systems for field trials.")); C1.NextDialogueRow = FName(TEXT("Kael_Calibrated"));
				FOnsetDialogueChoice C2; C2.ResponseText = FText::FromString(TEXT("What tactical doctrine should I apply?")); C2.NextDialogueRow = FName(TEXT("Kael_Lore"));
				FOnsetDialogueChoice C3; C3.ResponseText = FText::FromString(TEXT("At ease."));
				AddDial(FName(TEXT("Kael_Intro")), TEXT("Instructor Kael"), TEXT("Watch your spacing, Cadet! In the field, hesitation gets you cleaved."), { C1, C2, C3 });
				AddDial(FName(TEXT("Kael_Calibrated")), TEXT("Instructor Kael"), TEXT("Telemetry confirms clean conduit flow. Report to Commander Vane at South Gate."), { C3 });
				FOnsetDialogueChoice CBack; CBack.ResponseText = FText::FromString(TEXT("Understood, Instructor.")); CBack.NextDialogueRow = FName(TEXT("Kael_Intro"));
				AddDial(FName(TEXT("Kael_Lore")), TEXT("Instructor Kael"), TEXT("Skirmish packs probe your defenses. Swarm pockets try to overwhelm your cooldowns."), { CBack, C3 });
			}

			// Bren
			{
				FOnsetDialogueChoice C1; C1.ResponseText = FText::FromString(TEXT("Requesting armor calibration and requisitions.")); C1.NextDialogueRow = FName(TEXT("Bren_Stock"));
				FOnsetDialogueChoice C2; C2.ResponseText = FText::FromString(TEXT("Just inspecting the armory."));
				AddDial(FName(TEXT("Bren_Intro")), TEXT("Quartermaster Bren"), TEXT("Standard-issue composite weave and high-frequency blades. What does House Aegis have on your manifest?"), { C1, C2 });
				AddDial(FName(TEXT("Bren_Stock")), TEXT("Quartermaster Bren"), TEXT("Complete combat assignments beyond the perimeter wall to log vouchers."), { C2 });
			}

			// Tam
			{
				FOnsetDialogueChoice C1; C1.ResponseText = FText::FromString(TEXT("I'll investigate the coolant lines. [Accept: Coolant Malfunction]")); C1.GrantQuestOnSelect = FName(TEXT("Q3_CoolantMalfunction")); C1.NextDialogueRow = FName(TEXT("Tam_Q3Accepted"));
				FOnsetDialogueChoice C2; C2.ResponseText = FText::FromString(TEXT("Hold your position, Tam."));
				AddDial(FName(TEXT("Tam_Intro")), TEXT("Operative Tam"), TEXT("Good to see friendly armor out here. The bio-growth was triggered by a leak from Sector 2."), { C1, C2 });
				AddDial(FName(TEXT("Tam_Q3Accepted")), TEXT("Operative Tam"), TEXT("Follow conduit causeway to the reservoir. Cycle Coolant Flow Console to drain the floodgate."), { C2 });
			}

			// Mira
			{
				FOnsetDialogueChoice C1; C1.ResponseText = FText::FromString(TEXT("We'll take that relay back. [Accept: Signal Interference]")); C1.GrantQuestOnSelect = FName(TEXT("Q4_SignalInterference")); C1.NextDialogueRow = FName(TEXT("Mira_Q4Accepted"));
				FOnsetDialogueChoice C2; C2.ResponseText = FText::FromString(TEXT("Eyes sharp, Officer."));
				AddDial(FName(TEXT("Mira_Intro")), TEXT("Recon Officer Mira"), TEXT("Outcast scavengers hijacked the high-frequency comms relay dish."), { C1, C2 });
				AddDial(FName(TEXT("Mira_Q4Accepted")), TEXT("Recon Officer Mira"), TEXT("Clear out Outcast raiders, activate East Airlock Override Console to reconnect transit."), { C2 });
			}

			// Governor Thorne
			{
				FOnsetDialogueChoice C1; C1.ResponseText = FText::FromString(TEXT("What is the Confederate Council's stance on this frontier?")); C1.NextDialogueRow = FName(TEXT("Thorne_Council"));
				FOnsetDialogueChoice C2; C2.ResponseText = FText::FromString(TEXT("Honored to serve, Governor."));
				AddDial(FName(TEXT("Thorne_Intro")), TEXT("Governor Thorne"), TEXT("Greetings, Citizen. Planetary administration monitors this academy closely. The frontier must hold."), { C1, C2 });
				AddDial(FName(TEXT("Thorne_Council")), TEXT("Governor Thorne"), TEXT("The Confederate Senate allocates resources only to sectors with demonstrated stability. Keep the supply conduits open."), { C2 });
			}

			// Senator Vass
			{
				FOnsetDialogueChoice C1; C1.ResponseText = FText::FromString(TEXT("Reporting on perimeter security to the High Council.")); C1.NextDialogueRow = FName(TEXT("Vass_Security"));
				FOnsetDialogueChoice C2; C2.ResponseText = FText::FromString(TEXT("My respects, Senator."));
				AddDial(FName(TEXT("Vass_Intro")), TEXT("Senator Vass"), TEXT("The Confederate High Council watches the Substrate anomalies with grave concern."), { C1, C2 });
				AddDial(FName(TEXT("Vass_Security")), TEXT("Senator Vass"), TEXT("Maintain your vigilance, Operative. The Senate will recognize those who defend our charter."), { C2 });
			}
			break;
		}
	}

	Table->GetPackage()->MarkPackageDirty();
	return true;
}

FString UOnsetQuestEditorWidget::ValidateTable(EOnsetQuestEditorMode Mode, const UDataTable* Table) const
{
	if (!Table)
	{
		return TEXT("Table is null.");
	}

	TArray<FString> Errors;

	switch (Mode)
	{
		case EOnsetQuestEditorMode::Quests:
		{
			for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
			{
				const FName RowName = Pair.Key;
				const FOnsetQuestDefinition* Quest = reinterpret_cast<const FOnsetQuestDefinition*>(Pair.Value);
				if (!Quest) continue;

				if (Quest->DisplayName.IsEmpty())
				{
					Errors.Add(FString::Printf(TEXT("Quest '%s': DisplayName is empty."), *RowName.ToString()));
				}
				if (Quest->Stages.Num() == 0)
				{
					Errors.Add(FString::Printf(TEXT("Quest '%s': has 0 stages."), *RowName.ToString()));
				}

				for (int32 StageIdx = 0; StageIdx < Quest->Stages.Num(); ++StageIdx)
				{
					const FOnsetQuestStageDefinition& Stage = Quest->Stages[StageIdx];
					if (Stage.Objectives.Num() == 0)
					{
						Errors.Add(FString::Printf(TEXT("Quest '%s' [Stage %d]: has 0 objectives."), *RowName.ToString(), StageIdx + 1));
					}
					for (int32 ObjIdx = 0; ObjIdx < Stage.Objectives.Num(); ++ObjIdx)
					{
						const FOnsetQuestObjectiveDefinition& Obj = Stage.Objectives[ObjIdx];
						if (Obj.Count < 1)
						{
							Errors.Add(FString::Printf(TEXT("Quest '%s' [Stage %d, Obj %d]: Count must be >= 1 (currently %d)."), *RowName.ToString(), StageIdx + 1, ObjIdx + 1, Obj.Count));
						}
						if (Obj.Type == EOnsetQuestObjectiveType::Kill && Obj.Target.IsNone())
						{
							Errors.Add(FString::Printf(TEXT("Quest '%s' [Stage %d, Obj %d]: Kill objective target is None."), *RowName.ToString(), StageIdx + 1, ObjIdx + 1));
						}
						if (Obj.Type == EOnsetQuestObjectiveType::Interact && Obj.Target.IsNone())
						{
							Errors.Add(FString::Printf(TEXT("Quest '%s' [Stage %d, Obj %d]: Interact objective target is None."), *RowName.ToString(), StageIdx + 1, ObjIdx + 1));
						}
						if (Obj.Type == EOnsetQuestObjectiveType::ReachLocation && Obj.Radius <= 0.0f)
						{
							Errors.Add(FString::Printf(TEXT("Quest '%s' [Stage %d, Obj %d]: ReachLocation Radius must be > 0."), *RowName.ToString(), StageIdx + 1, ObjIdx + 1));
						}
					}
				}
			}
			break;
		}
		case EOnsetQuestEditorMode::NPCs:
		{
			for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
			{
				const FName RowName = Pair.Key;
				const FOnsetNPCDefinition* NPC = reinterpret_cast<const FOnsetNPCDefinition*>(Pair.Value);
				if (!NPC) continue;

				if (NPC->DisplayName.IsEmpty())
				{
					Errors.Add(FString::Printf(TEXT("NPC '%s': DisplayName is empty."), *RowName.ToString()));
				}
				if (NPC->DefaultDialogueRow.IsNone())
				{
					Errors.Add(FString::Printf(TEXT("NPC '%s': DefaultDialogueRow is not set."), *RowName.ToString()));
				}
			}
			break;
		}
		case EOnsetQuestEditorMode::Dialogues:
		{
			for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
			{
				const FName RowName = Pair.Key;
				const FOnsetDialogueRow* Dial = reinterpret_cast<const FOnsetDialogueRow*>(Pair.Value);
				if (!Dial) continue;

				if (Dial->DialogueText.IsEmpty())
				{
					Errors.Add(FString::Printf(TEXT("Dialogue '%s': DialogueText is empty."), *RowName.ToString()));
				}
				for (int32 ChoiceIdx = 0; ChoiceIdx < Dial->Choices.Num(); ++ChoiceIdx)
				{
					const FOnsetDialogueChoice& Choice = Dial->Choices[ChoiceIdx];
					if (Choice.ResponseText.IsEmpty())
					{
						Errors.Add(FString::Printf(TEXT("Dialogue '%s' [Choice %d]: ResponseText is empty."), *RowName.ToString(), ChoiceIdx + 1));
					}
				}
			}
			break;
		}
	}

	return Errors.Num() > 0 ? FString::Join(Errors, TEXT("\n")) : FString();
}

void UOnsetQuestEditorWidget::UpdateStatusText(const FText& Text, const FLinearColor& Color)
{
	if (StatusText)
	{
		StatusText->SetText(Text);
		StatusText->SetColorAndOpacity(FSlateColor(Color));
	}
}
