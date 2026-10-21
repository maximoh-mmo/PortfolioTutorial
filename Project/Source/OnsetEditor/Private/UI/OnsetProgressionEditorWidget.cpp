// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/OnsetProgressionEditorWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Combat/OnsetEquipmentLibrary.h"
#include "Combat/OnsetLootLibrary.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Data/OnsetClassInfoTypes.h"
#include "Data/OnsetEquipmentTypes.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "PackageTools.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "Styling/StyleDefaults.h"
#include "UI/OnsetNotifyDetailsView.h"

namespace OnsetProgressionEditorWidgetConstants
{
	constexpr float ColumnWidth = 380.0f;
}

static FName ProgressionNextWidgetName(const FString& Base)
{
	static int32 Counter = 0;
	return FName(*FString::Printf(TEXT("%s_%d"), *Base, ++Counter));
}

static FButtonStyle MakeProgressionButtonSubtle(const FLinearColor& FillColor)
{
	FButtonStyle Style;
	FSlateBrush Brush;
	Brush.TintColor = FSlateColor(FillColor);
	Style.SetNormal(Brush);

	FSlateBrush Hover = Brush;
	Hover.TintColor = FSlateColor(FillColor * 1.25f);
	Style.SetHovered(Hover);

	FSlateBrush Pressed = Brush;
	Pressed.TintColor = FSlateColor(FillColor * 0.85f);
	Style.SetPressed(Pressed);

	Style.SetNormalPadding(FMargin(8.0f, 4.0f));
	Style.SetPressedPadding(FMargin(8.0f, 5.0f));
	return Style;
}

static UButton* MakeProgressionActionBtn(UWidgetTree* WidgetTree, const FText& Label, const FString& BaseName, const FLinearColor& Color)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ProgressionNextWidgetName(BaseName + TEXT("_Btn")));
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), ProgressionNextWidgetName(BaseName + TEXT("_Label")));
	Text->SetText(Label);
	Text->SetJustification(ETextJustify::Center);
	Text->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 10));
	Button->SetContent(Text);
	Button->SetStyle(MakeProgressionButtonSubtle(Color));
	return Button;
}

static UVerticalBox* MakeProgressionCard(UWidgetTree* WidgetTree, const FText& Title, const FLinearColor& HeaderTint)
{
	UVerticalBox* Card = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), ProgressionNextWidgetName(TEXT("Card")));
	
	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), ProgressionNextWidgetName(TEXT("CardTitle")));
	TitleText->SetText(Title);
	TitleText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 11));
	TitleText->SetColorAndOpacity(FSlateColor(HeaderTint));

	UVerticalBoxSlot* TitleSlot = Card->AddChildToVerticalBox(TitleText);
	TitleSlot->SetPadding(FMargin(2.0f, 6.0f, 2.0f, 4.0f));

	return Card;
}

static UTextBlock* AddMetricRow(UWidgetTree* WidgetTree, UVerticalBox* TargetCard, const FText& Label, const FString& BaseName)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), ProgressionNextWidgetName(BaseName + TEXT("_Row")));
	
	UTextBlock* LabelBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), ProgressionNextWidgetName(BaseName + TEXT("_Lbl")));
	LabelBlock->SetText(Label);
	LabelBlock->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10));
	LabelBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.75f, 0.75f, 1.0f)));

	UTextBlock* ValBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), ProgressionNextWidgetName(BaseName + TEXT("_Val")));
	ValBlock->SetText(FText::FromString(TEXT("—")));
	ValBlock->SetJustification(ETextJustify::Right);
	ValBlock->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 10));
	ValBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));

	Row->AddChildToHorizontalBox(LabelBlock)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Row->AddChildToHorizontalBox(ValBlock)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UVerticalBoxSlot* Slot = TargetCard->AddChildToVerticalBox(Row);
	Slot->SetPadding(FMargin(4.0f, 1.0f, 4.0f, 1.0f));

	return ValBlock;
}

void UOnsetProgressionEditorWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildUI();
	LoadCurrentConfig();
	RecalculateSimulation();
}

void UOnsetProgressionEditorWidget::NativeDestruct()
{
	PersistOnClose();
	Super::NativeDestruct();
}

void UOnsetProgressionEditorWidget::OpenEditor()
{
	BuildUI();
	LoadCurrentConfig();
	RecalculateSimulation();
}

void UOnsetProgressionEditorWidget::BuildUI()
{
	if (WidgetTree && WidgetTree->RootWidget)
	{
		return;
	}

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("WorkbenchRoot"));

	// --- Header Banner ---
	UHorizontalBox* HeaderBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeaderBar"));
	UTextBlock* MainTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WorkbenchTitle"));
	MainTitle->SetText(NSLOCTEXT("OnsetWorkbench", "Title", "⚖️ ONSET PROGRESSION & BALANCE WORKBENCH"));
	MainTitle->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 14));
	MainTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.75f, 1.0f, 1.0f)));
	HeaderBar->AddChildToHorizontalBox(MainTitle);

	UVerticalBoxSlot* HeaderSlot = RootBox->AddChildToVerticalBox(HeaderBar);
	HeaderSlot->SetPadding(FMargin(12.0f, 8.0f, 12.0f, 6.0f));

	// --- 3-Pane Horizontal Layout ---
	UHorizontalBox* PanesRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PanesRow"));

	// -------------------------------------------------------------
	// PANE 1: CURVE & STATS
	// -------------------------------------------------------------
	UScrollBox* Pane1Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("Pane1Scroll"));
	Pane1Scroll->SetAlwaysShowScrollbar(true);
	UVerticalBox* Pane1Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Pane1Box"));
	Pane1Scroll->AddChild(Pane1Box);

	UVerticalBox* CurveHeaderCard = MakeProgressionCard(WidgetTree, NSLOCTEXT("OnsetWorkbench", "Pane1Title", "1. CURVE & STATS"), FLinearColor(0.2f, 0.7f, 1.0f, 1.0f));
	Pane1Box->AddChildToVerticalBox(CurveHeaderCard);

	CurvePropertyView = WidgetTree->ConstructWidget<UOnsetNotifyDetailsView>(UOnsetNotifyDetailsView::StaticClass(), TEXT("CurvePropertyView"));
	CurvePropertyView->OnPropertyEdited.AddUObject(this, &UOnsetProgressionEditorWidget::HandleCurvePropertyChanged);
	Pane1Box->AddChildToVerticalBox(CurvePropertyView);

	UVerticalBox* CurveReadoutCard = MakeProgressionCard(WidgetTree, NSLOCTEXT("OnsetWorkbench", "CurveReadoutTitle", "Leveling Curve Readouts"), FLinearColor(0.6f, 0.8f, 1.0f, 1.0f));
	TotalXpText = AddMetricRow(WidgetTree, CurveReadoutCard, NSLOCTEXT("OnsetWorkbench", "TotalXp", "Total XP Required:"), TEXT("TotalXp"));
	CumulativeKillsText = AddMetricRow(WidgetTree, CurveReadoutCard, NSLOCTEXT("OnsetWorkbench", "CumKills", "Cumulative Kills:"), TEXT("CumKills"));
	Pane1Box->AddChildToVerticalBox(CurveReadoutCard);

	UVerticalBox* TierRefCard = MakeProgressionCard(WidgetTree, NSLOCTEXT("OnsetWorkbench", "TierRefTitle", "Tier Progression Brackets"), FLinearColor(0.7f, 0.7f, 0.7f, 1.0f));
	UTextBlock* TierRefText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TierRefText"));
	TierRefText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 9));
	TierRefText->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f)));
	TierRefText->SetText(FText::FromString(TEXT("T0: Lvl 1–4   | Starter\nT1: Lvl 5–19  | Common\nT2: Lvl 20–49 | Uncommon\nT3: Lvl 50–99 | Rare\nT4: Lvl 100–199 | Epic\nT5: Lvl 200   | Legendary / Cap")));
	TierRefCard->AddChildToVerticalBox(TierRefText)->SetPadding(FMargin(4.0f, 2.0f, 4.0f, 4.0f));
	Pane1Box->AddChildToVerticalBox(TierRefCard);

	UHorizontalBoxSlot* Pane1Slot = PanesRow->AddChildToHorizontalBox(Pane1Scroll);
	Pane1Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Pane1Slot->SetPadding(FMargin(6.0f, 2.0f, 6.0f, 2.0f));

	// -------------------------------------------------------------
	// PANE 2: COMBAT SIMULATOR ("THE HOLODECK")
	// -------------------------------------------------------------
	UScrollBox* Pane2Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("Pane2Scroll"));
	Pane2Scroll->SetAlwaysShowScrollbar(true);
	UVerticalBox* Pane2Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Pane2Box"));
	Pane2Scroll->AddChild(Pane2Box);

	UVerticalBox* HolodeckHeaderCard = MakeProgressionCard(WidgetTree, NSLOCTEXT("OnsetWorkbench", "Pane2Title", "2. COMBAT SIMULATOR (HOLODECK)"), FLinearColor(0.3f, 0.9f, 0.4f, 1.0f));
	Pane2Box->AddChildToVerticalBox(HolodeckHeaderCard);

	HolodeckPropertyView = WidgetTree->ConstructWidget<UOnsetNotifyDetailsView>(UOnsetNotifyDetailsView::StaticClass(), TEXT("HolodeckPropertyView"));
	HolodeckPropertyView->OnPropertyEdited.AddUObject(this, &UOnsetProgressionEditorWidget::HandleHolodeckPropertyChanged);
	Pane2Box->AddChildToVerticalBox(HolodeckPropertyView);

	UVerticalBox* SimCard = MakeProgressionCard(WidgetTree, NSLOCTEXT("OnsetWorkbench", "SimCardTitle", "Combat Simulation Results"), FLinearColor(0.5f, 0.95f, 0.6f, 1.0f));
	PlayerDamageHitText = AddMetricRow(WidgetTree, SimCard, NSLOCTEXT("OnsetWorkbench", "RawDmg", "Player Damage / Hit:"), TEXT("RawDmg"));
	EnemyMitigationText = AddMetricRow(WidgetTree, SimCard, NSLOCTEXT("OnsetWorkbench", "EnemyMit", "Enemy Physical Mit:"), TEXT("EnemyMit"));
	NetHitDamageText = AddMetricRow(WidgetTree, SimCard, NSLOCTEXT("OnsetWorkbench", "NetDmg", "Net Hit Damage:"), TEXT("NetDmg"));
	AttacksPerSecondText = AddMetricRow(WidgetTree, SimCard, NSLOCTEXT("OnsetWorkbench", "APS", "Attacks / Sec:"), TEXT("APS"));
	PlayerDpsText = AddMetricRow(WidgetTree, SimCard, NSLOCTEXT("OnsetWorkbench", "PlayerDPS", "Player Net DPS:"), TEXT("PlayerDPS"));
	TimeToKillText = AddMetricRow(WidgetTree, SimCard, NSLOCTEXT("OnsetWorkbench", "TTK", "Time-To-Kill (TTK):"), TEXT("TTK"));
	HitsToKillText = AddMetricRow(WidgetTree, SimCard, NSLOCTEXT("OnsetWorkbench", "HTK", "Hits-To-Kill (HTK):"), TEXT("HTK"));
	EnemyDpsText = AddMetricRow(WidgetTree, SimCard, NSLOCTEXT("OnsetWorkbench", "EnemyDPS", "Enemy DPS to Player:"), TEXT("EnemyDPS"));
	PlayerSurvivalText = AddMetricRow(WidgetTree, SimCard, NSLOCTEXT("OnsetWorkbench", "Survival", "Player Survival Time:"), TEXT("Survival"));
	HitsToKillPlayerText = AddMetricRow(WidgetTree, SimCard, NSLOCTEXT("OnsetWorkbench", "HitsToDie", "Hits To Kill Player:"), TEXT("HitsToDie"));
	Pane2Box->AddChildToVerticalBox(SimCard);

	UVerticalBox* PacingCard = MakeProgressionCard(WidgetTree, NSLOCTEXT("OnsetWorkbench", "PacingCardTitle", "Pacing Rating"), FLinearColor(0.9f, 0.8f, 0.2f, 1.0f));
	PacingRatingText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PacingRatingText"));
	PacingRatingText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 12));
	PacingRatingText->SetText(FText::FromString(TEXT("OPTIMAL")));
	PacingRatingText->SetColorAndOpacity(FSlateColor(FLinearColor::Green));
	PacingCard->AddChildToVerticalBox(PacingRatingText)->SetPadding(FMargin(6.0f, 4.0f, 6.0f, 6.0f));
	Pane2Box->AddChildToVerticalBox(PacingCard);

	UHorizontalBoxSlot* Pane2Slot = PanesRow->AddChildToHorizontalBox(Pane2Scroll);
	Pane2Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Pane2Slot->SetPadding(FMargin(6.0f, 2.0f, 6.0f, 2.0f));

	// -------------------------------------------------------------
	// PANE 3: LOOT & DROP PACING
	// -------------------------------------------------------------
	UScrollBox* Pane3Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("Pane3Scroll"));
	Pane3Scroll->SetAlwaysShowScrollbar(true);
	UVerticalBox* Pane3Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Pane3Box"));
	Pane3Scroll->AddChild(Pane3Box);

	UVerticalBox* LootHeaderCard = MakeProgressionCard(WidgetTree, NSLOCTEXT("OnsetWorkbench", "Pane3Title", "3. LOOT & DROP PACING"), FLinearColor(1.0f, 0.6f, 0.2f, 1.0f));
	Pane3Box->AddChildToVerticalBox(LootHeaderCard);

	LootPropertyView = WidgetTree->ConstructWidget<UOnsetNotifyDetailsView>(UOnsetNotifyDetailsView::StaticClass(), TEXT("LootPropertyView"));
	LootPropertyView->OnPropertyEdited.AddUObject(this, &UOnsetProgressionEditorWidget::HandleLootPropertyChanged);
	Pane3Box->AddChildToVerticalBox(LootPropertyView);

	UVerticalBox* BudgetCard = MakeProgressionCard(WidgetTree, NSLOCTEXT("OnsetWorkbench", "BudgetCardTitle", "Tier Kill Budget & Pacing"), FLinearColor(1.0f, 0.75f, 0.4f, 1.0f));
	LevelSpanText = AddMetricRow(WidgetTree, BudgetCard, NSLOCTEXT("OnsetWorkbench", "LvlSpan", "Tier Level Span (ΔL):"), TEXT("LvlSpan"));
	KillBudgetText = AddMetricRow(WidgetTree, BudgetCard, NSLOCTEXT("OnsetWorkbench", "KillBudg", "Tier Kill Budget:"), TEXT("KillBudg"));
	PlaytimeText = AddMetricRow(WidgetTree, BudgetCard, NSLOCTEXT("OnsetWorkbench", "Playtime", "Playtime (@200k/hr):"), TEXT("Playtime"));
	RequiredDropRateText = AddMetricRow(WidgetTree, BudgetCard, NSLOCTEXT("OnsetWorkbench", "ReqDrop", "Required Drop Rate P:"), TEXT("ReqDrop"));
	Pane3Box->AddChildToVerticalBox(BudgetCard);

	UVerticalBox* MonteCarloCard = MakeProgressionCard(WidgetTree, NSLOCTEXT("OnsetWorkbench", "MCCardTitle", "Monte Carlo Roll Validator"), FLinearColor(0.85f, 0.5f, 1.0f, 1.0f));
	
	UButton* RunMCBtn = MakeProgressionActionBtn(WidgetTree, NSLOCTEXT("OnsetWorkbench", "RunMC", "🎲 Run Monte Carlo Test"), TEXT("RunMC"), FLinearColor(0.35f, 0.2f, 0.55f, 1.0f));
	RunMCBtn->OnClicked.AddDynamic(this, &UOnsetProgressionEditorWidget::RunMonteCarloTest);
	MonteCarloCard->AddChildToVerticalBox(RunMCBtn)->SetPadding(FMargin(2.0f, 4.0f, 2.0f, 6.0f));

	MonteCarloDropsText = AddMetricRow(WidgetTree, MonteCarloCard, NSLOCTEXT("OnsetWorkbench", "MCDrops", "Simulated Drops:"), TEXT("MCDrops"));
	MonteCarloFrequencyText = AddMetricRow(WidgetTree, MonteCarloCard, NSLOCTEXT("OnsetWorkbench", "MCFreq", "Drop Frequency:"), TEXT("MCFreq"));
	MonteCarloDroughtText = AddMetricRow(WidgetTree, MonteCarloCard, NSLOCTEXT("OnsetWorkbench", "MCDrought", "Max Drought Streak:"), TEXT("MCDrought"));

	MonteCarloStatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MonteCarloStatusText"));
	MonteCarloStatusText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 10));
	MonteCarloStatusText->SetText(FText::FromString(TEXT("Status: Ready for simulation test.")));
	MonteCarloStatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f)));
	MonteCarloCard->AddChildToVerticalBox(MonteCarloStatusText)->SetPadding(FMargin(4.0f, 4.0f, 4.0f, 4.0f));

	Pane3Box->AddChildToVerticalBox(MonteCarloCard);

	UHorizontalBoxSlot* Pane3Slot = PanesRow->AddChildToHorizontalBox(Pane3Scroll);
	Pane3Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Pane3Slot->SetPadding(FMargin(6.0f, 2.0f, 6.0f, 2.0f));

	RootBox->AddChildToVerticalBox(PanesRow)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// -------------------------------------------------------------
	// BOTTOM ACTION BAR
	// -------------------------------------------------------------
	UHorizontalBox* BottomBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BottomBar"));

	StatusBarText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusBarText"));
	StatusBarText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10));
	StatusBarText->SetText(FText::FromString(TEXT("Ready. All DataTables validated.")));
	StatusBarText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.85f, 0.7f, 1.0f)));
	BottomBar->AddChildToHorizontalBox(StatusBarText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UButton* PreviewBtn = MakeProgressionActionBtn(WidgetTree, NSLOCTEXT("OnsetWorkbench", "Preview", "Preview Diffs"), TEXT("Preview"), FLinearColor(0.2f, 0.4f, 0.6f, 1.0f));
	PreviewBtn->OnClicked.AddDynamic(this, &UOnsetProgressionEditorWidget::PreviewChanges);
	BottomBar->AddChildToHorizontalBox(PreviewBtn)->SetPadding(FMargin(4.0f, 0.0f, 4.0f, 0.0f));

	UButton* RevertBtn = MakeProgressionActionBtn(WidgetTree, NSLOCTEXT("OnsetWorkbench", "Revert", "Revert Changes"), TEXT("Revert"), FLinearColor(0.5f, 0.25f, 0.25f, 1.0f));
	RevertBtn->OnClicked.AddDynamic(this, &UOnsetProgressionEditorWidget::RevertChanges);
	BottomBar->AddChildToHorizontalBox(RevertBtn)->SetPadding(FMargin(4.0f, 0.0f, 4.0f, 0.0f));

	UButton* CommitBtn = MakeProgressionActionBtn(WidgetTree, NSLOCTEXT("OnsetWorkbench", "Commit", "Commit Changes to Engine.ini & DataTables"), TEXT("Commit"), FLinearColor(0.15f, 0.55f, 0.25f, 1.0f));
	CommitBtn->OnClicked.AddDynamic(this, &UOnsetProgressionEditorWidget::CommitChanges);
	BottomBar->AddChildToHorizontalBox(CommitBtn)->SetPadding(FMargin(4.0f, 0.0f, 0.0f, 0.0f));

	UVerticalBoxSlot* BottomSlot = RootBox->AddChildToVerticalBox(BottomBar);
	BottomSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	BottomSlot->SetPadding(FMargin(12.0f, 8.0f, 12.0f, 8.0f));

	WidgetTree->RootWidget = RootBox;
}

void UOnsetProgressionEditorWidget::LoadCurrentConfig()
{
	if (!CurveWrapper)
	{
		CurveWrapper = NewObject<UOnsetProgressionCurveWrapper>(this, TEXT("CurveWrapper"));
	}
	if (!HolodeckWrapper)
	{
		HolodeckWrapper = NewObject<UOnsetCombatHolodeckWrapper>(this, TEXT("HolodeckWrapper"));
	}
	if (!LootPacingWrapper)
	{
		LootPacingWrapper = NewObject<UOnsetLootPacingWrapper>(this, TEXT("LootPacingWrapper"));
	}

	// Read from DefaultEngine.ini [Onset.Gameplay]
	GConfig->GetFloat(TEXT("Onset.Gameplay"), TEXT("XpBase"), CurveWrapper->XPBase, GEngineIni);
	GConfig->GetFloat(TEXT("Onset.Gameplay"), TEXT("XpGrowth"), CurveWrapper->XPGrowth, GEngineIni);
	GConfig->GetInt(TEXT("Onset.Gameplay"), TEXT("KillsPerLevel"), CurveWrapper->KillsPerLevel, GEngineIni);
	GConfig->GetInt(TEXT("Onset.Gameplay"), TEXT("StatPointsPerLevel"), CurveWrapper->StatPointsPerLevel, GEngineIni);
	GConfig->GetFloat(TEXT("Onset.Gameplay"), TEXT("KDefense"), CurveWrapper->KDefense, GEngineIni);
	GConfig->GetFloat(TEXT("Onset.Gameplay"), TEXT("KZoneTierScale"), CurveWrapper->KZoneTierScale, GEngineIni);
	GConfig->GetFloat(TEXT("Onset.Gameplay"), TEXT("HealthPerVitality"), CurveWrapper->HealthPerVitality, GEngineIni);

	if (CurveWrapper->XPBase <= 0.0f) CurveWrapper->XPBase = 50.0f;
	if (CurveWrapper->XPGrowth <= 0.0f) CurveWrapper->XPGrowth = 0.06f;
	if (CurveWrapper->KillsPerLevel <= 0) CurveWrapper->KillsPerLevel = 180;
	if (CurveWrapper->StatPointsPerLevel <= 0) CurveWrapper->StatPointsPerLevel = 3;
	if (CurveWrapper->KDefense <= 0.0f) CurveWrapper->KDefense = 100.0f;
	if (CurveWrapper->KZoneTierScale <= 0.0f) CurveWrapper->KZoneTierScale = 1.0f;
	if (CurveWrapper->HealthPerVitality <= 0.0f) CurveWrapper->HealthPerVitality = 10.0f;

	OriginalXPBase = CurveWrapper->XPBase;
	OriginalXPGrowth = CurveWrapper->XPGrowth;
	OriginalKillsPerLevel = CurveWrapper->KillsPerLevel;
	OriginalStatPointsPerLevel = CurveWrapper->StatPointsPerLevel;
	OriginalKDefense = CurveWrapper->KDefense;
	OriginalKZoneTierScale = CurveWrapper->KZoneTierScale;
	OriginalHealthPerVitality = CurveWrapper->HealthPerVitality;

	if (CurvePropertyView)
	{
		CurvePropertyView->SetObject(CurveWrapper);
	}
	if (HolodeckPropertyView)
	{
		HolodeckPropertyView->SetObject(HolodeckWrapper);
	}
	if (LootPropertyView)
	{
		LootPropertyView->SetObject(LootPacingWrapper);
	}

	UpdateLootTierDefaults(LootPacingWrapper->Tier);
	bIsDirty = false;
}

void UOnsetProgressionEditorWidget::HandleCurvePropertyChanged()
{
	bIsDirty = true;
	if (StatusBarText)
	{
		StatusBarText->SetText(FText::FromString(TEXT("⚠️ Modified: Curve settings changed (Unsaved).")));
		StatusBarText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.8f, 0.2f, 1.0f)));
	}
	RecalculateSimulation();
}

void UOnsetProgressionEditorWidget::HandleHolodeckPropertyChanged()
{
	RecalculateSimulation();
}

void UOnsetProgressionEditorWidget::HandleLootPropertyChanged()
{
	if (LootPacingWrapper && LootPacingWrapper->Tier != CachedLootTier)
	{
		UpdateLootTierDefaults(LootPacingWrapper->Tier);
	}
	RecalculateSimulation();
}

void UOnsetProgressionEditorWidget::UpdateLootTierDefaults(int32 NewTier)
{
	CachedLootTier = NewTier;
	if (!LootPacingWrapper)
	{
		return;
	}

	switch (NewTier)
	{
	case 0:
		LootPacingWrapper->MinLevel = 1;
		LootPacingWrapper->MaxLevel = 4;
		LootPacingWrapper->TargetTierDrops = 15;
		LootPacingWrapper->LootTableRowName = TEXT("ST_Tier0_Equip");
		break;
	case 1:
		LootPacingWrapper->MinLevel = 5;
		LootPacingWrapper->MaxLevel = 19;
		LootPacingWrapper->TargetTierDrops = 55;
		LootPacingWrapper->LootTableRowName = TEXT("ST_Tier1_Equip");
		break;
	case 2:
		LootPacingWrapper->MinLevel = 20;
		LootPacingWrapper->MaxLevel = 49;
		LootPacingWrapper->TargetTierDrops = 90;
		LootPacingWrapper->LootTableRowName = TEXT("ST_Tier2_Equip");
		break;
	case 3:
		LootPacingWrapper->MinLevel = 50;
		LootPacingWrapper->MaxLevel = 99;
		LootPacingWrapper->TargetTierDrops = 130;
		LootPacingWrapper->LootTableRowName = TEXT("ST_Tier3_Equip");
		break;
	case 4:
		LootPacingWrapper->MinLevel = 100;
		LootPacingWrapper->MaxLevel = 199;
		LootPacingWrapper->TargetTierDrops = 220;
		LootPacingWrapper->LootTableRowName = TEXT("ST_Tier4_Equip");
		break;
	case 5:
	default:
		LootPacingWrapper->MinLevel = 200;
		LootPacingWrapper->MaxLevel = 200;
		LootPacingWrapper->TargetTierDrops = 300;
		LootPacingWrapper->LootTableRowName = TEXT("ST_Tier5_Equip");
		break;
	}

	if (LootPropertyView)
	{
		LootPropertyView->SetObject(LootPacingWrapper);
	}
}

void UOnsetProgressionEditorWidget::RecalculateSimulation()
{
	if (!CurveWrapper || !HolodeckWrapper || !LootPacingWrapper)
	{
		return;
	}

	// 1. Leveling Curve Readouts
	const float CurrentLevelXp = FOnsetProgressionSimulator::CalculateXPRequired(HolodeckWrapper->PlayerLevel, CurveWrapper->XPBase, CurveWrapper->XPGrowth);
	const float CumulativeXP = FOnsetProgressionSimulator::CalculateCumulativeXP(HolodeckWrapper->PlayerLevel, CurveWrapper->XPBase, CurveWrapper->XPGrowth);
	const int32 CumulativeKills = (HolodeckWrapper->PlayerLevel - 1) * CurveWrapper->KillsPerLevel;

	if (TotalXpText)
	{
		TotalXpText->SetText(FText::FromString(FString::Printf(TEXT("%s XP (Lvl %d: %s)"), 
			*FText::AsNumber((int64)CumulativeXP).ToString(),
			HolodeckWrapper->PlayerLevel,
			*FText::AsNumber((int64)CurrentLevelXp).ToString())));
	}
	if (CumulativeKillsText)
	{
		CumulativeKillsText->SetText(FText::FromString(FString::Printf(TEXT("%s kills"), *FText::AsNumber(CumulativeKills).ToString())));
	}

	// 2. Combat Holodeck Simulation
	const FOnsetCharacterClassInfo* ClassInfo = UOnsetEquipmentLibrary::GetClassInfo(HolodeckWrapper->PlayerClass);

	// Standard gear tier budgets (from Docs/combat-tuning-calculator.xlsx)
	TArray<FOnsetEquipmentDefinition> Gear;
	FOnsetEquipmentDefinition Weapon;
	Weapon.Slot = EOnsetEquipmentSlot::Weapon;
	Weapon.Archetype = HolodeckWrapper->WeaponArchetype;

	FOnsetEquipmentDefinition Armor;
	Armor.Slot = EOnsetEquipmentSlot::Chest;

	switch (HolodeckWrapper->GearTier)
	{
	case 0:
		Weapon.WeaponDamage = 20.0f;
		Armor.DefenseBonus = 0.0f;
		break;
	case 1:
		Weapon.WeaponDamage = 35.0f;
		Armor.DefenseBonus = 30.0f;
		Armor.StrengthBonus = 10.0f;
		Armor.VitalityBonus = 10.0f;
		Armor.AgilityBonus = 5.0f;
		Armor.LuckBonus = 5.0f;
		break;
	case 2:
		Weapon.WeaponDamage = 65.0f;
		Armor.DefenseBonus = 70.0f;
		Armor.StrengthBonus = 25.0f;
		Armor.VitalityBonus = 25.0f;
		Armor.AgilityBonus = 15.0f;
		Armor.LuckBonus = 15.0f;
		break;
	case 3:
		Weapon.WeaponDamage = 120.0f;
		Armor.DefenseBonus = 160.0f;
		Armor.StrengthBonus = 60.0f;
		Armor.VitalityBonus = 60.0f;
		Armor.AgilityBonus = 35.0f;
		Armor.LuckBonus = 35.0f;
		break;
	case 4:
		Weapon.WeaponDamage = 240.0f;
		Armor.DefenseBonus = 360.0f;
		Armor.StrengthBonus = 130.0f;
		Armor.VitalityBonus = 130.0f;
		Armor.AgilityBonus = 75.0f;
		Armor.LuckBonus = 75.0f;
		break;
	case 5:
	default:
		Weapon.WeaponDamage = 400.0f;
		Armor.DefenseBonus = 700.0f;
		Armor.StrengthBonus = 250.0f;
		Armor.VitalityBonus = 250.0f;
		Armor.AgilityBonus = 150.0f;
		Armor.LuckBonus = 150.0f;
		break;
	}

	Gear.Add(Weapon);
	Gear.Add(Armor);

	// Distribute level-up stat points according to class identity
	const int32 TotalStatPts = (HolodeckWrapper->PlayerLevel - 1) * CurveWrapper->StatPointsPerLevel;
	TMap<FName, float> AllocMap;
	switch (HolodeckWrapper->PlayerClass)
	{
	case EOnsetCharacterClass::Tank:
		AllocMap.Add(TEXT("Vitality"), (float)TotalStatPts * 0.50f);
		AllocMap.Add(TEXT("Defense"), (float)TotalStatPts * 0.30f);
		AllocMap.Add(TEXT("Strength"), (float)TotalStatPts * 0.20f);
		break;
	case EOnsetCharacterClass::Ranged:
		AllocMap.Add(TEXT("Strength"), (float)TotalStatPts * 0.50f);
		AllocMap.Add(TEXT("Agility"), (float)TotalStatPts * 0.30f);
		AllocMap.Add(TEXT("Luck"), (float)TotalStatPts * 0.20f);
		break;
	case EOnsetCharacterClass::Support:
		AllocMap.Add(TEXT("Intellect"), (float)TotalStatPts * 0.50f);
		AllocMap.Add(TEXT("Vitality"), (float)TotalStatPts * 0.30f);
		AllocMap.Add(TEXT("Agility"), (float)TotalStatPts * 0.20f);
		break;
	case EOnsetCharacterClass::DPS:
	default:
		AllocMap.Add(TEXT("Strength"), (float)TotalStatPts * 0.50f);
		AllocMap.Add(TEXT("Vitality"), (float)TotalStatPts * 0.30f);
		AllocMap.Add(TEXT("Agility"), (float)TotalStatPts * 0.20f);
		break;
	}

	FPlayerCombatSnapshot Snapshot = FOnsetProgressionSimulator::EvaluatePlayer(
		HolodeckWrapper->PlayerClass,
		ClassInfo,
		HolodeckWrapper->PlayerLevel,
		Gear,
		AllocMap,
		CurveWrapper->KDefense,
		CurveWrapper->KZoneTierScale,
		CurveWrapper->HealthPerVitality
	);

	// Lookup enemy
	const FOnsetEnemyStats* EnemyRow = UOnsetEquipmentLibrary::GetEnemyStats(HolodeckWrapper->SelectedEnemyRow);
	FOnsetEnemyStats TargetEnemy;
	if (EnemyRow)
	{
		TargetEnemy = *EnemyRow;
	}
	else
	{
		TargetEnemy.Level = HolodeckWrapper->PlayerLevel;
		TargetEnemy.MaxHealth = 100.0f * FMath::Pow(1.15f, (float)HolodeckWrapper->GearTier);
		TargetEnemy.DamageBase = 25.0f * FMath::Pow(1.15f, (float)HolodeckWrapper->GearTier);
		TargetEnemy.Defense = 10.0f + 15.0f * HolodeckWrapper->GearTier;
		TargetEnemy.WeaponArchetype = EOnsetWeaponArchetype::Sword;
	}

	FSimulationResult Sim = FOnsetProgressionSimulator::SimulateCombat(
		Snapshot,
		TargetEnemy,
		CurveWrapper->KDefense,
		CurveWrapper->KZoneTierScale,
		HolodeckWrapper->EnemyRankMultiplier
	);

	if (PlayerDamageHitText) PlayerDamageHitText->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), Snapshot.RawDamagePerHit)));
	if (EnemyMitigationText) EnemyMitigationText->SetText(FText::FromString(FString::Printf(TEXT("%.1f%%"), Sim.EnemyMitigationPct * 100.0f)));
	if (NetHitDamageText) NetHitDamageText->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), Sim.PlayerNetHitDamage)));
	if (AttacksPerSecondText) AttacksPerSecondText->SetText(FText::FromString(FString::Printf(TEXT("%.2f /s"), Snapshot.AttacksPerSecond)));
	if (PlayerDpsText) PlayerDpsText->SetText(FText::FromString(FString::Printf(TEXT("%.1f DPS"), Sim.PlayerDPS)));
	if (TimeToKillText) TimeToKillText->SetText(FText::FromString(FString::Printf(TEXT("%.1f s"), Sim.TimeToKillSeconds)));
	if (HitsToKillText) HitsToKillText->SetText(FText::FromString(FString::Printf(TEXT("%d hits"), Sim.HitsToKill)));
	if (EnemyDpsText) EnemyDpsText->SetText(FText::FromString(FString::Printf(TEXT("%.1f DPS"), Sim.EnemyDPS)));
	if (PlayerSurvivalText) PlayerSurvivalText->SetText(FText::FromString(FString::Printf(TEXT("%.1f s"), Sim.PlayerSurvivalSeconds)));
	if (HitsToKillPlayerText) HitsToKillPlayerText->SetText(FText::FromString(FString::Printf(TEXT("%d hits"), Sim.HitsToKillPlayer)));

	if (PacingRatingText)
	{
		PacingRatingText->SetText(FText::FromString(Sim.PacingStatus));
		PacingRatingText->SetColorAndOpacity(FSlateColor(Sim.PacingColor));
	}

	// 3. Loot Pacing Metrics
	const int32 KillBudget = FOnsetLootPacingCalculator::CalculateKillBudget(LootPacingWrapper->MinLevel, LootPacingWrapper->MaxLevel, CurveWrapper->KillsPerLevel);
	const float DropRate = FOnsetLootPacingCalculator::DeriveRequiredDropChance(LootPacingWrapper->TargetTierDrops, KillBudget);
	const float PlaytimeHours = (float)KillBudget / 200.0f;
	const int32 Span = FMath::Max(1, LootPacingWrapper->MaxLevel - LootPacingWrapper->MinLevel + 1);

	if (LevelSpanText) LevelSpanText->SetText(FText::FromString(FString::Printf(TEXT("%d levels (Lvl %d-%d)"), Span, LootPacingWrapper->MinLevel, LootPacingWrapper->MaxLevel)));
	if (KillBudgetText) KillBudgetText->SetText(FText::FromString(FString::Printf(TEXT("%s kills"), *FText::AsNumber(KillBudget).ToString())));
	if (PlaytimeText) PlaytimeText->SetText(FText::FromString(FString::Printf(TEXT("%.1f hrs"), PlaytimeHours)));
	if (RequiredDropRateText) RequiredDropRateText->SetText(FText::FromString(FString::Printf(TEXT("%.2f%% (Target %d drops)"), DropRate * 100.0f, LootPacingWrapper->TargetTierDrops)));
}

void UOnsetProgressionEditorWidget::RunMonteCarloTest()
{
	if (!LootPacingWrapper || !CurveWrapper)
	{
		return;
	}

	const int32 KillBudget = FOnsetLootPacingCalculator::CalculateKillBudget(LootPacingWrapper->MinLevel, LootPacingWrapper->MaxLevel, CurveWrapper->KillsPerLevel);
	const float TargetDropRate = FOnsetLootPacingCalculator::DeriveRequiredDropChance(LootPacingWrapper->TargetTierDrops, KillBudget);

	UDataTable* LootTable = UOnsetLootLibrary::GetLootTable();
	FMonteCarloResult Result = FOnsetLootPacingCalculator::RunSimulation(
		LootTable,
		LootPacingWrapper->LootTableRowName,
		LootPacingWrapper->MinLevel,
		LootPacingWrapper->Iterations,
		TargetDropRate
	);

	if (MonteCarloDropsText)
	{
		MonteCarloDropsText->SetText(FText::FromString(FString::Printf(TEXT("%d items (%.2f%%)"), Result.TotalDropsAcquired, Result.DropsPerKillPct)));
	}
	if (MonteCarloFrequencyText)
	{
		MonteCarloFrequencyText->SetText(FText::FromString(FString::Printf(TEXT("1 in %.1f kills"), Result.AverageKillsBetweenDrops)));
	}
	if (MonteCarloDroughtText)
	{
		MonteCarloDroughtText->SetText(FText::FromString(FString::Printf(TEXT("%d consecutive kills"), Result.MaxDroughtStreak)));
	}
	if (MonteCarloStatusText)
	{
		MonteCarloStatusText->SetText(FText::FromString(Result.SummaryStatus));
		MonteCarloStatusText->SetColorAndOpacity(Result.bMetPacingTarget
			? FSlateColor(FLinearColor(0.2f, 0.85f, 0.3f, 1.0f))
			: FSlateColor(FLinearColor(0.95f, 0.65f, 0.1f, 1.0f)));
	}
}

void UOnsetProgressionEditorWidget::PreviewChanges()
{
	if (!CurveWrapper)
	{
		return;
	}

	FString DiffText = TEXT("=== PROGRESSION & BALANCE WORKBENCH DIFF PREVIEW ===\n\n");
	bool bAnyChanges = false;

	if (!FMath::IsNearlyEqual(CurveWrapper->XPBase, OriginalXPBase, 0.001f))
	{
		DiffText += FString::Printf(TEXT("• XPBase: %.2f  →  %.2f\n"), OriginalXPBase, CurveWrapper->XPBase);
		bAnyChanges = true;
	}
	if (!FMath::IsNearlyEqual(CurveWrapper->XPGrowth, OriginalXPGrowth, 0.0001f))
	{
		DiffText += FString::Printf(TEXT("• XPGrowth: %.4f  →  %.4f\n"), OriginalXPGrowth, CurveWrapper->XPGrowth);
		bAnyChanges = true;
	}
	if (CurveWrapper->KillsPerLevel != OriginalKillsPerLevel)
	{
		DiffText += FString::Printf(TEXT("• KillsPerLevel: %d  →  %d\n"), OriginalKillsPerLevel, CurveWrapper->KillsPerLevel);
		bAnyChanges = true;
	}
	if (CurveWrapper->StatPointsPerLevel != OriginalStatPointsPerLevel)
	{
		DiffText += FString::Printf(TEXT("• StatPointsPerLevel: %d  →  %d\n"), OriginalStatPointsPerLevel, CurveWrapper->StatPointsPerLevel);
		bAnyChanges = true;
	}
	if (!FMath::IsNearlyEqual(CurveWrapper->KDefense, OriginalKDefense, 0.001f))
	{
		DiffText += FString::Printf(TEXT("• KDefense: %.1f  →  %.1f\n"), OriginalKDefense, CurveWrapper->KDefense);
		bAnyChanges = true;
	}
	if (!FMath::IsNearlyEqual(CurveWrapper->KZoneTierScale, OriginalKZoneTierScale, 0.001f))
	{
		DiffText += FString::Printf(TEXT("• KZoneTierScale: %.2f  →  %.2f\n"), OriginalKZoneTierScale, CurveWrapper->KZoneTierScale);
		bAnyChanges = true;
	}
	if (!FMath::IsNearlyEqual(CurveWrapper->HealthPerVitality, OriginalHealthPerVitality, 0.001f))
	{
		DiffText += FString::Printf(TEXT("• HealthPerVitality: %.1f  →  %.1f\n"), OriginalHealthPerVitality, CurveWrapper->HealthPerVitality);
		bAnyChanges = true;
	}

	if (!bAnyChanges)
	{
		DiffText += TEXT("No curve or config settings have been modified from DefaultEngine.ini.");
	}
	else
	{
		DiffText += TEXT("\nTarget Configuration File:\nProject/Config/DefaultEngine.ini [Onset.Gameplay]");
	}

	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(DiffText), FText::FromString(TEXT("Workbench Diff Preview")));
}

void UOnsetProgressionEditorWidget::RevertChanges()
{
	LoadCurrentConfig();
	RecalculateSimulation();
	if (StatusBarText)
	{
		StatusBarText->SetText(FText::FromString(TEXT("Reverted all curve settings back to DefaultEngine.ini baseline.")));
		StatusBarText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.85f, 0.7f, 1.0f)));
	}
}

void UOnsetProgressionEditorWidget::CommitChanges()
{
	if (!CurveWrapper)
	{
		return;
	}

	// 1. Update DefaultEngine.ini on disk
	const FString DefaultEngineIniPath = FPaths::SourceConfigDir() / TEXT("DefaultEngine.ini");
	GConfig->SetFloat(TEXT("Onset.Gameplay"), TEXT("XpBase"), CurveWrapper->XPBase, DefaultEngineIniPath);
	GConfig->SetFloat(TEXT("Onset.Gameplay"), TEXT("XpGrowth"), CurveWrapper->XPGrowth, DefaultEngineIniPath);
	GConfig->SetInt(TEXT("Onset.Gameplay"), TEXT("KillsPerLevel"), CurveWrapper->KillsPerLevel, DefaultEngineIniPath);
	GConfig->SetInt(TEXT("Onset.Gameplay"), TEXT("StatPointsPerLevel"), CurveWrapper->StatPointsPerLevel, DefaultEngineIniPath);
	GConfig->SetFloat(TEXT("Onset.Gameplay"), TEXT("KDefense"), CurveWrapper->KDefense, DefaultEngineIniPath);
	GConfig->SetFloat(TEXT("Onset.Gameplay"), TEXT("KZoneTierScale"), CurveWrapper->KZoneTierScale, DefaultEngineIniPath);
	GConfig->SetFloat(TEXT("Onset.Gameplay"), TEXT("HealthPerVitality"), CurveWrapper->HealthPerVitality, DefaultEngineIniPath);
	GConfig->Flush(false, DefaultEngineIniPath);

	// 2. Also update in-memory GEngineIni so running session reflects immediately
	GConfig->SetFloat(TEXT("Onset.Gameplay"), TEXT("XpBase"), CurveWrapper->XPBase, GEngineIni);
	GConfig->SetFloat(TEXT("Onset.Gameplay"), TEXT("XpGrowth"), CurveWrapper->XPGrowth, GEngineIni);
	GConfig->SetInt(TEXT("Onset.Gameplay"), TEXT("KillsPerLevel"), CurveWrapper->KillsPerLevel, GEngineIni);
	GConfig->SetInt(TEXT("Onset.Gameplay"), TEXT("StatPointsPerLevel"), CurveWrapper->StatPointsPerLevel, GEngineIni);
	GConfig->SetFloat(TEXT("Onset.Gameplay"), TEXT("KDefense"), CurveWrapper->KDefense, GEngineIni);
	GConfig->SetFloat(TEXT("Onset.Gameplay"), TEXT("KZoneTierScale"), CurveWrapper->KZoneTierScale, GEngineIni);
	GConfig->SetFloat(TEXT("Onset.Gameplay"), TEXT("HealthPerVitality"), CurveWrapper->HealthPerVitality, GEngineIni);
	GConfig->Flush(false, GEngineIni);

	// 3. Update baselines
	OriginalXPBase = CurveWrapper->XPBase;
	OriginalXPGrowth = CurveWrapper->XPGrowth;
	OriginalKillsPerLevel = CurveWrapper->KillsPerLevel;
	OriginalStatPointsPerLevel = CurveWrapper->StatPointsPerLevel;
	OriginalKDefense = CurveWrapper->KDefense;
	OriginalKZoneTierScale = CurveWrapper->KZoneTierScale;
	OriginalHealthPerVitality = CurveWrapper->HealthPerVitality;

	bIsDirty = false;

	// 4. Save any dirty DataTables
	TArray<UObject*> TablesToSave;
	if (UDataTable* EnemyTable = UOnsetEquipmentLibrary::GetEnemyStatsTable())
	{
		if (EnemyTable->GetPackage()->IsDirty())
		{
			TablesToSave.Add(EnemyTable);
		}
	}
	if (UDataTable* EquipTable = UOnsetEquipmentLibrary::GetEquipmentTable())
	{
		if (EquipTable->GetPackage()->IsDirty())
		{
			TablesToSave.Add(EquipTable);
		}
	}
	if (UDataTable* LootTable = UOnsetLootLibrary::GetLootTable())
	{
		if (LootTable->GetPackage()->IsDirty())
		{
			TablesToSave.Add(LootTable);
		}
	}

	if (TablesToSave.Num() > 0)
	{
		UPackageTools::SavePackagesForObjects(TablesToSave);
	}

	if (StatusBarText)
	{
		StatusBarText->SetText(FText::FromString(TEXT("✅ Committed changes to DefaultEngine.ini and saved DataTables.")));
		StatusBarText->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.9f, 0.3f, 1.0f)));
	}
}

void UOnsetProgressionEditorWidget::PersistOnClose()
{
	if (!bIsDirty)
	{
		return;
	}

	CommitChanges();
}
