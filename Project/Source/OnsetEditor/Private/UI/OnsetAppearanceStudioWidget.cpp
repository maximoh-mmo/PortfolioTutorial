// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/OnsetAppearanceStudioWidget.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/DetailsView.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "LevelEditorViewport.h"
#include "ObjectTools.h"
#include "OnsetAnimationProfile.h"
#include "OnsetAppearanceCatalog.h"
#include "OnsetAppearanceComponent.h"
#include "OnsetAppearancePreset.h"
#include "OnsetAppearancePreviewActor.h"
#include "OnsetAuthoredAppearance.h"
#include "PackageTools.h"
#include "Selection.h"
#include "UI/OnsetNotifyDetailsView.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace OnsetStudioConstants
{
	constexpr float ListEntryHeight = 24.0f;
	constexpr float IdWidth = 180.0f;
	constexpr float NameWidth = 180.0f;
	constexpr float CategoryWidth = 110.0f;
	constexpr float FactionWidth = 110.0f;

	const FString AppearanceAssetPath = TEXT("/Game/OnsetCharacterCreation/Appearances");
	const FString AnimationProfileAssetPath = TEXT("/Game/OnsetCharacterCreation/AnimationProfiles");
}

static FName StudioNextWidgetName(const FString& Base)
{
	static int32 Counter = 0;
	return FName(*FString::Printf(TEXT("%s_%d"), *Base, ++Counter));
}

static FButtonStyle MakeStudioFlatStyle()
{
	FButtonStyle Style;
	Style.SetNormal(*FStyleDefaults::GetNoBrush());

	FSlateBrush Hover = *FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	Hover.TintColor = FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.12f));
	FSlateBrush Pressed = Hover;
	Pressed.TintColor = FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.22f));

	Style.SetHovered(Hover);
	Style.SetPressed(Pressed);
	Style.SetDisabled(*FStyleDefaults::GetNoBrush());
	Style.SetNormalPadding(FMargin(6.0f, 2.0f));
	Style.SetPressedPadding(FMargin(6.0f, 3.0f));
	return Style;
}

static FButtonStyle MakeStudioRowStyle(bool bSelected)
{
	FButtonStyle Style = MakeStudioFlatStyle();
	Style.SetNormalPadding(FMargin(2.0f, 2.0f));
	Style.SetPressedPadding(FMargin(2.0f, 3.0f));
	if (bSelected)
	{
		FSlateBrush Selected = *FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
		Selected.TintColor = FSlateColor(FLinearColor(0.12f, 0.40f, 0.75f, 0.45f));
		FSlateBrush SelectedHover = Selected;
		SelectedHover.TintColor = FSlateColor(FLinearColor(0.18f, 0.48f, 0.85f, 0.55f));
		Style.SetNormal(Selected);
		Style.SetHovered(SelectedHover);
		Style.SetPressed(SelectedHover);
	}
	return Style;
}

static USizeBox* MakeStudioCell(UWidgetTree* WidgetTree, const FString& BaseName, float Width, const FText& Text, bool bHeader, ETextJustify::Type Justification)
{
	USizeBox* CellBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), StudioNextWidgetName(BaseName + TEXT("_Cell")));
	CellBox->SetWidthOverride(Width);

	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), StudioNextWidgetName(BaseName + TEXT("_Label")));
	TextBlock->SetText(Text);
	TextBlock->SetJustification(Justification);
	TextBlock->SetFont(bHeader ? FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 10) : FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10));
	if (bHeader)
	{
		TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.75f, 0.85f, 1.0f)));
	}

	CellBox->AddChild(TextBlock);
	return CellBox;
}

static UButton* MakeStudioTextButton(UWidgetTree* WidgetTree, const FText& Label, const FString& BaseName)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), StudioNextWidgetName(BaseName + TEXT("_Button")));
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), StudioNextWidgetName(BaseName + TEXT("_Label")));
	Text->SetText(Label);
	Text->SetJustification(ETextJustify::Center);
	Text->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10));
	Button->SetContent(Text);
	Button->SetStyle(MakeStudioFlatStyle());
	return Button;
}

void UOnsetAppearanceStudioWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureStarterAssetsSeeded();
	BuildUI();
	RefreshAssetList();
}

void UOnsetAppearanceStudioWidget::NativeDestruct()
{
	SaveAllAssets();
	CleanupPreviewActor();
	Super::NativeDestruct();
}

void UOnsetAppearanceStudioWidget::OpenEditor()
{
	EnsureStarterAssetsSeeded();
	BuildUI();
	RefreshAssetList();
}

void UOnsetAppearanceStudioWidget::BuildUI()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("StudioRootBox"));

	// --- Top: Mode Tabs (Appearances vs Animation Profiles) ---
	UHorizontalBox* ModeTabRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ModeTabRow"));
	ModeButtons.Reset();

	const struct FStudioModeConfig
	{
		EOnsetAppearanceStudioMode Mode;
		FText Label;
	} ModeConfigs[] = {
		{ EOnsetAppearanceStudioMode::Appearances, NSLOCTEXT("OnsetStudio", "ModeAppearances", "Authored Appearances Library") },
		{ EOnsetAppearanceStudioMode::AnimationProfiles, NSLOCTEXT("OnsetStudio", "ModeAnimProfiles", "Animation Profiles Library") }
	};

	for (const auto& Cfg : ModeConfigs)
	{
		UOnsetAppearanceStudioModeButton* Btn = NewObject<UOnsetAppearanceStudioModeButton>(this);
		Btn->Mode = Cfg.Mode;
		Btn->OnClicked.AddDynamic(Btn, &UOnsetAppearanceStudioModeButton::HandleClicked);
		Btn->OnModeSelected.AddUObject(this, &UOnsetAppearanceStudioWidget::OnModeButtonClicked);

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), StudioNextWidgetName(TEXT("ModeBtnLabel")));
		Label->SetText(Cfg.Label);
		Label->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 10));
		Btn->SetContent(Label);
		Btn->SetStyle(MakeStudioFlatStyle());

		ModeTabRow->AddChildToHorizontalBox(Btn)->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
		ModeButtons.Add(Btn);
	}

	RootBox->AddChildToVerticalBox(ModeTabRow)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));

	// --- Middle: 3-column content (Asset Browser | Viewport / Preview Controls | Details Panel) ---
	UHorizontalBox* ContentRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ContentRow"));

	// 1. Left Panel: Asset Browser
	UVerticalBox* LeftPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftPanel"));

	HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeaderRow"));
	LeftPanel->AddChildToVerticalBox(HeaderRow);

	UScrollBox* ListScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("AssetListScroll"));
	ListScroll->SetAlwaysShowScrollbar(true);
	ListScroll->SetAlwaysShowScrollbarTrack(true);
	LeftPanel->AddChildToVerticalBox(ListScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	AssetListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("AssetListBox"));
	ListScroll->AddChild(AssetListBox);

	UHorizontalBox* ListActionRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ListActionRow"));
	UButton* NewBtn = MakeStudioTextButton(WidgetTree, NSLOCTEXT("OnsetStudio", "New", "+ New"), TEXT("New"));
	NewBtn->OnClicked.AddDynamic(this, &UOnsetAppearanceStudioWidget::CreateNewAsset);
	ListActionRow->AddChildToHorizontalBox(NewBtn)->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));

	UButton* DuplicateBtn = MakeStudioTextButton(WidgetTree, NSLOCTEXT("OnsetStudio", "Duplicate", "Duplicate Variant"), TEXT("Duplicate"));
	DuplicateBtn->OnClicked.AddDynamic(this, &UOnsetAppearanceStudioWidget::DuplicateSelectedAsset);
	ListActionRow->AddChildToHorizontalBox(DuplicateBtn)->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));

	UButton* DeleteBtn = MakeStudioTextButton(WidgetTree, NSLOCTEXT("OnsetStudio", "Delete", "- Delete"), TEXT("Delete"));
	DeleteBtn->OnClicked.AddDynamic(this, &UOnsetAppearanceStudioWidget::DeleteSelectedAsset);
	ListActionRow->AddChildToHorizontalBox(DeleteBtn);

	LeftPanel->AddChildToVerticalBox(ListActionRow)->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));

	// 2. Center Panel: Live Viewport & Preview Controls
	UVerticalBox* CenterPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CenterPanel"));

	UTextBlock* CenterHeader = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CenterHeader"));
	CenterHeader->SetText(NSLOCTEXT("OnsetStudio", "CenterHeader", "3D Viewport & Preview Controls"));
	CenterHeader->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 10));
	CenterHeader->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.85f, 1.0f, 1.0f)));
	CenterPanel->AddChildToVerticalBox(CenterHeader)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));

	PreviewInfoText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PreviewInfoText"));
	PreviewInfoText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Italic"), 9));
	PreviewInfoText->SetText(NSLOCTEXT("OnsetStudio", "PreviewReady", "Preview Stage: Ready"));
	CenterPanel->AddChildToVerticalBox(PreviewInfoText)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

	// Viewport staging actions
	UHorizontalBox* StageRow1 = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StageRow1"));
	UButton* FocusBtn = MakeStudioTextButton(WidgetTree, NSLOCTEXT("OnsetStudio", "SpawnFocus", "Focus In Viewport"), TEXT("Focus"));
	FocusBtn->OnClicked.AddDynamic(this, &UOnsetAppearanceStudioWidget::SpawnOrFocusPreviewActor);
	StageRow1->AddChildToHorizontalBox(FocusBtn)->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));

	UButton* PlayPauseBtn = MakeStudioTextButton(WidgetTree, NSLOCTEXT("OnsetStudio", "PlayPause", "Play / Pause"), TEXT("PlayPause"));
	PlayPauseBtn->OnClicked.AddDynamic(this, &UOnsetAppearanceStudioWidget::TogglePreviewPlayback);
	StageRow1->AddChildToHorizontalBox(PlayPauseBtn);
	CenterPanel->AddChildToVerticalBox(StageRow1)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));

	// Turntable & Rotation
	UHorizontalBox* TurnRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TurnRow"));
	UButton* TurnLeftBtn = MakeStudioTextButton(WidgetTree, NSLOCTEXT("OnsetStudio", "TurnLeft", "Turn -45°"), TEXT("TurnLeft"));
	TurnLeftBtn->OnClicked.AddDynamic(this, &UOnsetAppearanceStudioWidget::RotatePreviewLeft);
	TurnRow->AddChildToHorizontalBox(TurnLeftBtn)->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));

	UButton* TurnRightBtn = MakeStudioTextButton(WidgetTree, NSLOCTEXT("OnsetStudio", "TurnRight", "Turn +45°"), TEXT("TurnRight"));
	TurnRightBtn->OnClicked.AddDynamic(this, &UOnsetAppearanceStudioWidget::RotatePreviewRight);
	TurnRow->AddChildToHorizontalBox(TurnRightBtn)->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));

	UButton* TurntableBtn = MakeStudioTextButton(WidgetTree, NSLOCTEXT("OnsetStudio", "Turntable", "Toggle Turntable"), TEXT("Turntable"));
	TurntableBtn->OnClicked.AddDynamic(this, &UOnsetAppearanceStudioWidget::ToggleTurntable);
	TurnRow->AddChildToHorizontalBox(TurntableBtn);
	CenterPanel->AddChildToVerticalBox(TurnRow)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));

	// Pose & Montage preview triggers
	UTextBlock* PoseHeader = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PoseHeader"));
	PoseHeader->SetText(NSLOCTEXT("OnsetStudio", "PoseHeader", "Postures & Protocol Previews:"));
	PoseHeader->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 9));
	CenterPanel->AddChildToVerticalBox(PoseHeader)->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 2.0f));

	UHorizontalBox* PoseRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PoseRow"));
	UButton* IdleBtn = MakeStudioTextButton(WidgetTree, NSLOCTEXT("OnsetStudio", "Idle", "Idle Stance"), TEXT("Idle"));
	IdleBtn->OnClicked.AddDynamic(this, &UOnsetAppearanceStudioWidget::PlayIdleStance);
	PoseRow->AddChildToHorizontalBox(IdleBtn)->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));

	UButton* SaluteBtn = MakeStudioTextButton(WidgetTree, NSLOCTEXT("OnsetStudio", "Salute", "Protocol Salute"), TEXT("Salute"));
	SaluteBtn->OnClicked.AddDynamic(this, &UOnsetAppearanceStudioWidget::PlayProtocolSalute);
	PoseRow->AddChildToHorizontalBox(SaluteBtn)->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));

	UButton* TalkBtn = MakeStudioTextButton(WidgetTree, NSLOCTEXT("OnsetStudio", "Talk", "Talk Gesture"), TEXT("Talk"));
	TalkBtn->OnClicked.AddDynamic(this, &UOnsetAppearanceStudioWidget::PlayTalkGesture);
	PoseRow->AddChildToHorizontalBox(TalkBtn)->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));

	UButton* TPoseBtn = MakeStudioTextButton(WidgetTree, NSLOCTEXT("OnsetStudio", "TPose", "T-Pose"), TEXT("TPose"));
	TPoseBtn->OnClicked.AddDynamic(this, &UOnsetAppearanceStudioWidget::ShowReferencePose);
	PoseRow->AddChildToHorizontalBox(TPoseBtn);
	CenterPanel->AddChildToVerticalBox(PoseRow)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));

	// Apply to Level Actor
	UButton* ApplyActorBtn = MakeStudioTextButton(WidgetTree, NSLOCTEXT("OnsetStudio", "ApplyActor", "Apply Recipe To Selected Level Actor"), TEXT("ApplyActor"));
	ApplyActorBtn->OnClicked.AddDynamic(this, &UOnsetAppearanceStudioWidget::ApplyToSelectedLevelActor);
	CenterPanel->AddChildToVerticalBox(ApplyActorBtn)->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));

	// 3. Right Panel: Details View
	PropertyView = WidgetTree->ConstructWidget<UOnsetNotifyDetailsView>(UOnsetNotifyDetailsView::StaticClass(), TEXT("PropertyView"));
	PropertyView->OnPropertyEdited.AddUObject(this, &UOnsetAppearanceStudioWidget::HandlePropertyEdited);

	ContentRow->AddChildToHorizontalBox(LeftPanel)->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	UHorizontalBoxSlot* CenterSlot = ContentRow->AddChildToHorizontalBox(CenterPanel);
	CenterSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	CenterSlot->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 0.0f));

	UHorizontalBoxSlot* RightSlot = ContentRow->AddChildToHorizontalBox(PropertyView);
	RightSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	RootBox->AddChildToVerticalBox(ContentRow)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// --- Status Text & Save Actions ---
	UHorizontalBox* BottomRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BottomRow"));

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	StatusText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10));
	StatusText->SetAutoWrapText(true);
	BottomRow->AddChildToHorizontalBox(StatusText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UButton* SaveAssetBtn = MakeStudioTextButton(WidgetTree, NSLOCTEXT("OnsetStudio", "SaveAsset", "Save Asset"), TEXT("SaveAsset"));
	SaveAssetBtn->OnClicked.AddDynamic(this, &UOnsetAppearanceStudioWidget::SaveSelectedAsset);
	BottomRow->AddChildToHorizontalBox(SaveAssetBtn)->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));

	UButton* SaveAllBtn = MakeStudioTextButton(WidgetTree, NSLOCTEXT("OnsetStudio", "SaveAll", "Save All"), TEXT("SaveAll"));
	SaveAllBtn->OnClicked.AddDynamic(this, &UOnsetAppearanceStudioWidget::SaveAllAssets);
	BottomRow->AddChildToHorizontalBox(SaveAllBtn);

	RootBox->AddChildToVerticalBox(BottomRow)->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));

	WidgetTree->RootWidget = RootBox;
	UpdateModeButtonHighlight();
}

void UOnsetAppearanceStudioWidget::UpdateModeButtonHighlight()
{
	for (UOnsetAppearanceStudioModeButton* Btn : ModeButtons)
	{
		if (Btn)
		{
			const bool bActive = (Btn->Mode == CurrentMode);
			Btn->SetStyle(bActive ? MakeStudioRowStyle(true) : MakeStudioFlatStyle());
		}
	}
}

void UOnsetAppearanceStudioWidget::OnModeButtonClicked(EOnsetAppearanceStudioMode Mode)
{
	if (CurrentMode == Mode) return;
	CurrentMode = Mode;
	CurrentSelectedObject = nullptr;
	CurrentSelectedId = NAME_None;
	UpdateModeButtonHighlight();
	RefreshAssetList();
}

void UOnsetAppearanceStudioWidget::RefreshAssetList()
{
	if (!HeaderRow || !AssetListBox || !WidgetTree) return;

	HeaderRow->ClearChildren();
	AssetListBox->ClearChildren();

	if (CurrentMode == EOnsetAppearanceStudioMode::Appearances)
	{
		HeaderRow->AddChild(MakeStudioCell(WidgetTree, TEXT("HeaderId"), OnsetStudioConstants::IdWidth, NSLOCTEXT("OnsetStudio", "ColId", "Appearance ID"), true, ETextJustify::Left));
		HeaderRow->AddChild(MakeStudioCell(WidgetTree, TEXT("HeaderName"), OnsetStudioConstants::NameWidth, NSLOCTEXT("OnsetStudio", "ColName", "Display Name"), true, ETextJustify::Left));
		HeaderRow->AddChild(MakeStudioCell(WidgetTree, TEXT("HeaderCategory"), OnsetStudioConstants::CategoryWidth, NSLOCTEXT("OnsetStudio", "ColCategory", "Category"), true, ETextJustify::Center));
		HeaderRow->AddChild(MakeStudioCell(WidgetTree, TEXT("HeaderFaction"), OnsetStudioConstants::FactionWidth, NSLOCTEXT("OnsetStudio", "ColFaction", "Faction"), true, ETextJustify::Center));

		TArray<FAssetData> AssetList;
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistryModule.Get().GetAssetsByPath(FName(*OnsetStudioConstants::AppearanceAssetPath), AssetList, true);

		UObject* FirstFound = nullptr;
		FName FirstId = NAME_None;

		for (const FAssetData& AssetData : AssetList)
		{
			UOnsetAuthoredAppearance* Appearance = Cast<UOnsetAuthoredAppearance>(AssetData.GetAsset());
			if (!Appearance) continue;

			const FName AppId = Appearance->AppearanceId.IsNone() ? AssetData.AssetName : Appearance->AppearanceId;
			if (!FirstFound)
			{
				FirstFound = Appearance;
				FirstId = AppId;
			}

			const bool bSelected = (AppId == CurrentSelectedId);

			UOnsetAppearanceAssetButton* RowBtn = NewObject<UOnsetAppearanceAssetButton>(this);
			RowBtn->AssetId = AppId;
			RowBtn->AssetObject = Appearance;
			RowBtn->OnClicked.AddDynamic(RowBtn, &UOnsetAppearanceAssetButton::HandleClicked);
			RowBtn->OnAssetSelected.AddUObject(this, &UOnsetAppearanceStudioWidget::OnAssetSelected);
			RowBtn->SetStyle(MakeStudioRowStyle(bSelected));

			UHorizontalBox* RowHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), StudioNextWidgetName(TEXT("RowHBox")));
			RowHBox->AddChild(MakeStudioCell(WidgetTree, TEXT("CellId"), OnsetStudioConstants::IdWidth, FText::FromName(AppId), false, ETextJustify::Left));
			RowHBox->AddChild(MakeStudioCell(WidgetTree, TEXT("CellName"), OnsetStudioConstants::NameWidth, Appearance->DisplayName, false, ETextJustify::Left));
			RowHBox->AddChild(MakeStudioCell(WidgetTree, TEXT("CellCategory"), OnsetStudioConstants::CategoryWidth, StaticEnum<EOnsetAppearanceCategory>()->GetDisplayNameTextByValue((int64)Appearance->Category), false, ETextJustify::Center));
			RowHBox->AddChild(MakeStudioCell(WidgetTree, TEXT("CellFaction"), OnsetStudioConstants::FactionWidth, FText::FromName(Appearance->FactionAffiliation), false, ETextJustify::Center));

			RowBtn->SetContent(RowHBox);
			AssetListBox->AddChildToVerticalBox(RowBtn);
		}

		if (!CurrentSelectedObject && FirstFound)
		{
			OnAssetSelected(FirstId, FirstFound);
		}
	}
	else
	{
		HeaderRow->AddChild(MakeStudioCell(WidgetTree, TEXT("HeaderProfileId"), OnsetStudioConstants::IdWidth + 40.0f, NSLOCTEXT("OnsetStudio", "ColProfileId", "Profile ID"), true, ETextJustify::Left));
		HeaderRow->AddChild(MakeStudioCell(WidgetTree, TEXT("HeaderProfileName"), OnsetStudioConstants::NameWidth + 40.0f, NSLOCTEXT("OnsetStudio", "ColProfileName", "Display Name"), true, ETextJustify::Left));
		HeaderRow->AddChild(MakeStudioCell(WidgetTree, TEXT("HeaderRig"), OnsetStudioConstants::CategoryWidth, NSLOCTEXT("OnsetStudio", "ColRig", "Rig Compatibility"), true, ETextJustify::Center));

		TArray<FAssetData> AssetList;
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistryModule.Get().GetAssetsByPath(FName(*OnsetStudioConstants::AnimationProfileAssetPath), AssetList, true);

		UObject* FirstFound = nullptr;
		FName FirstId = NAME_None;

		for (const FAssetData& AssetData : AssetList)
		{
			UOnsetAnimationProfile* Profile = Cast<UOnsetAnimationProfile>(AssetData.GetAsset());
			if (!Profile) continue;

			const FName ProfileId = Profile->ProfileId.IsNone() ? AssetData.AssetName : Profile->ProfileId;
			if (!FirstFound)
			{
				FirstFound = Profile;
				FirstId = ProfileId;
			}

			const bool bSelected = (ProfileId == CurrentSelectedId);

			UOnsetAppearanceAssetButton* RowBtn = NewObject<UOnsetAppearanceAssetButton>(this);
			RowBtn->AssetId = ProfileId;
			RowBtn->AssetObject = Profile;
			RowBtn->OnClicked.AddDynamic(RowBtn, &UOnsetAppearanceAssetButton::HandleClicked);
			RowBtn->OnAssetSelected.AddUObject(this, &UOnsetAppearanceStudioWidget::OnAssetSelected);
			RowBtn->SetStyle(MakeStudioRowStyle(bSelected));

			UHorizontalBox* RowHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), StudioNextWidgetName(TEXT("ProfileRowHBox")));
			RowHBox->AddChild(MakeStudioCell(WidgetTree, TEXT("CellProfId"), OnsetStudioConstants::IdWidth + 40.0f, FText::FromName(ProfileId), false, ETextJustify::Left));
			RowHBox->AddChild(MakeStudioCell(WidgetTree, TEXT("CellProfName"), OnsetStudioConstants::NameWidth + 40.0f, Profile->DisplayName, false, ETextJustify::Left));
			RowHBox->AddChild(MakeStudioCell(WidgetTree, TEXT("CellRig"), OnsetStudioConstants::CategoryWidth, FText::FromName(Profile->RigCompatibility), false, ETextJustify::Center));

			RowBtn->SetContent(RowHBox);
			AssetListBox->AddChildToVerticalBox(RowBtn);
		}

		if (!CurrentSelectedObject && FirstFound)
		{
			OnAssetSelected(FirstId, FirstFound);
		}
	}
}

void UOnsetAppearanceStudioWidget::OnAssetSelected(FName AssetId, UObject* AssetObject)
{
	CurrentSelectedId = AssetId;
	CurrentSelectedObject = AssetObject;

	if (PropertyView)
	{
		PropertyView->SetObject(AssetObject);
	}

	UpdatePreviewActorVisuals();
	UpdateStatusText(FText::Format(NSLOCTEXT("OnsetStudio", "Selected", "Viewing '{0}'"), FText::FromName(AssetId)), FLinearColor(0.3f, 0.8f, 1.0f));
}

void UOnsetAppearanceStudioWidget::HandlePropertyEdited()
{
	if (CurrentSelectedObject)
	{
		CurrentSelectedObject->MarkPackageDirty();
	}
	UpdatePreviewActorVisuals();
	UpdateStatusText(FText::Format(NSLOCTEXT("OnsetStudio", "Edited", "Edited '{0}' (Unsaved changes)"), FText::FromName(CurrentSelectedId)), FLinearColor(0.9f, 0.7f, 0.2f));
}

void UOnsetAppearanceStudioWidget::UpdateStatusText(const FText& Text, const FLinearColor& Color)
{
	if (StatusText)
	{
		StatusText->SetText(Text);
		StatusText->SetColorAndOpacity(FSlateColor(Color));
	}
}

AOnsetAppearancePreviewActor* UOnsetAppearanceStudioWidget::GetOrCreatePreviewActor()
{
	if (CachedPreviewActor.IsValid())
	{
		return CachedPreviewActor.Get();
	}

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World) return nullptr;

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, AOnsetAppearancePreviewActor::StaticClass(), FoundActors);
	if (FoundActors.Num() > 0)
	{
		AOnsetAppearancePreviewActor* Existing = Cast<AOnsetAppearancePreviewActor>(FoundActors[0]);
		CachedPreviewActor = Existing;
		return Existing;
	}

	// Spawn new preview actor in level as transient
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.ObjectFlags |= RF_Transient;

	FVector SpawnLoc = FVector::ZeroVector;
	FRotator SpawnRot = FRotator(0.f, -90.f, 0.f);

	if (GEditor && GEditor->GetActiveViewport())
	{
		FEditorViewportClient* Client = (FEditorViewportClient*)GEditor->GetActiveViewport()->GetClient();
		if (Client)
		{
			SpawnLoc = Client->GetViewLocation() + Client->GetViewRotation().Vector() * 300.0f;
			SpawnLoc.Z -= 80.0f;
			SpawnRot = FRotator(0.f, Client->GetViewRotation().Yaw + 180.0f, 0.f);
		}
	}

	AOnsetAppearancePreviewActor* NewActor = World->SpawnActor<AOnsetAppearancePreviewActor>(AOnsetAppearancePreviewActor::StaticClass(), SpawnLoc, SpawnRot, SpawnParams);
	CachedPreviewActor = NewActor;
	return NewActor;
}

void UOnsetAppearanceStudioWidget::CleanupPreviewActor()
{
	if (CachedPreviewActor.IsValid())
	{
		CachedPreviewActor->Destroy();
		CachedPreviewActor.Reset();
	}

	CleanupAllPreviewActors();
}

void UOnsetAppearanceStudioWidget::CleanupAllPreviewActors()
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World) return;

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, AOnsetAppearancePreviewActor::StaticClass(), FoundActors);
	for (AActor* Actor : FoundActors)
	{
		if (Actor && IsValid(Actor) && !Actor->IsPendingKillPending())
		{
			Actor->Destroy();
		}
	}
}

void UOnsetAppearanceStudioWidget::UpdatePreviewActorVisuals()
{
	AOnsetAppearancePreviewActor* PreviewActor = GetOrCreatePreviewActor();
	if (!PreviewActor) return;

	if (CurrentMode == EOnsetAppearanceStudioMode::Appearances)
	{
		if (UOnsetAuthoredAppearance* Appearance = Cast<UOnsetAuthoredAppearance>(CurrentSelectedObject))
		{
			PreviewActor->LoadAuthoredAppearance(Appearance);
			if (PreviewInfoText)
			{
				PreviewInfoText->SetText(FText::Format(NSLOCTEXT("OnsetStudio", "PreviewApp", "Previewing Appearance: {0} ({1})"), Appearance->DisplayName, FText::FromName(Appearance->AppearanceId)));
			}
		}
	}
	else
	{
		if (UOnsetAnimationProfile* Profile = Cast<UOnsetAnimationProfile>(CurrentSelectedObject))
		{
			PreviewActor->LoadAnimationProfile(Profile);
			if (PreviewInfoText)
			{
				PreviewInfoText->SetText(FText::Format(NSLOCTEXT("OnsetStudio", "PreviewProf", "Previewing Animation Profile: {0}"), Profile->DisplayName));
			}
		}
	}
}

void UOnsetAppearanceStudioWidget::SpawnOrFocusPreviewActor()
{
	AOnsetAppearancePreviewActor* PreviewActor = GetOrCreatePreviewActor();
	if (!PreviewActor)
	{
		UpdateStatusText(NSLOCTEXT("OnsetStudio", "NoWorldFocus", "Could not acquire preview actor in editor world."), FLinearColor::Red);
		return;
	}

	if (GEditor)
	{
		GEditor->SelectNone(false, true);
		GEditor->SelectActor(PreviewActor, true, true);
		GEditor->MoveViewportCamerasToActor(*PreviewActor, false);
		UpdateStatusText(NSLOCTEXT("OnsetStudio", "Focused", "Viewport camera focused on Preview Actor."), FLinearColor(0.2f, 0.9f, 0.4f));
	}
}

void UOnsetAppearanceStudioWidget::TogglePreviewPlayback()
{
	if (AOnsetAppearancePreviewActor* PreviewActor = GetOrCreatePreviewActor())
	{
		if (PreviewActor->bPreviewPlaying)
		{
			PreviewActor->PausePreview();
			UpdateStatusText(NSLOCTEXT("OnsetStudio", "Paused", "Preview animation paused."));
		}
		else
		{
			PreviewActor->PlayPreview();
			UpdateStatusText(NSLOCTEXT("OnsetStudio", "Playing", "Preview animation playing."));
		}
	}
}

void UOnsetAppearanceStudioWidget::PlayIdleStance()
{
	if (AOnsetAppearancePreviewActor* PreviewActor = GetOrCreatePreviewActor())
	{
		if (UOnsetAuthoredAppearance* App = Cast<UOnsetAuthoredAppearance>(CurrentSelectedObject))
		{
			if (UOnsetAnimationProfile* Prof = App->AnimationProfile.LoadSynchronous())
			{
				PreviewActor->LoadAnimationProfile(Prof);
			}
		}
		PreviewActor->PlayPreview();
	}
}

void UOnsetAppearanceStudioWidget::PlayProtocolSalute()
{
	if (AOnsetAppearancePreviewActor* PreviewActor = GetOrCreatePreviewActor())
	{
		UOnsetAnimationProfile* Prof = nullptr;
		if (UOnsetAuthoredAppearance* App = Cast<UOnsetAuthoredAppearance>(CurrentSelectedObject))
		{
			Prof = App->AnimationProfile.LoadSynchronous();
		}
		else
		{
			Prof = Cast<UOnsetAnimationProfile>(CurrentSelectedObject);
		}

		if (Prof && !Prof->SaluteMontage.IsNull())
		{
			UpdateStatusText(NSLOCTEXT("OnsetStudio", "Saluting", "Playing protocol salute sequence."), FLinearColor(0.2f, 0.8f, 1.0f));
		}
		else
		{
			UpdateStatusText(NSLOCTEXT("OnsetStudio", "NoSalute", "No salute montage assigned to this profile."), FLinearColor(0.9f, 0.6f, 0.2f));
		}
	}
}

void UOnsetAppearanceStudioWidget::PlayTalkGesture()
{
	if (AOnsetAppearancePreviewActor* PreviewActor = GetOrCreatePreviewActor())
	{
		PreviewActor->NextSampleAnimation();
	}
}

void UOnsetAppearanceStudioWidget::ShowReferencePose()
{
	if (AOnsetAppearancePreviewActor* PreviewActor = GetOrCreatePreviewActor())
	{
		PreviewActor->ShowReferencePose();
		UpdateStatusText(NSLOCTEXT("OnsetStudio", "TPoseSet", "Reference pose (T-Pose) evaluated."));
	}
}

void UOnsetAppearanceStudioWidget::RotatePreview(float Degrees)
{
	if (AOnsetAppearancePreviewActor* PreviewActor = GetOrCreatePreviewActor())
	{
		PreviewActor->RotatePreview(Degrees);
	}
}

void UOnsetAppearanceStudioWidget::RotatePreviewLeft()
{
	RotatePreview(-45.0f);
}

void UOnsetAppearanceStudioWidget::RotatePreviewRight()
{
	RotatePreview(45.0f);
}

void UOnsetAppearanceStudioWidget::ToggleTurntable()
{
	if (AOnsetAppearancePreviewActor* PreviewActor = GetOrCreatePreviewActor())
	{
		PreviewActor->bTurntableEnabled = !PreviewActor->bTurntableEnabled;
		UpdateStatusText(PreviewActor->bTurntableEnabled
			? NSLOCTEXT("OnsetStudio", "TurntableOn", "Turntable rotation ENABLED (360° inspection).")
			: NSLOCTEXT("OnsetStudio", "TurntableOff", "Turntable rotation DISABLED."));
	}
}

void UOnsetAppearanceStudioWidget::ApplyToSelectedLevelActor()
{
	UOnsetAuthoredAppearance* Appearance = Cast<UOnsetAuthoredAppearance>(CurrentSelectedObject);
	if (!Appearance)
	{
		UpdateStatusText(NSLOCTEXT("OnsetStudio", "SelectAppFirst", "Select an authored appearance first."), FLinearColor(0.9f, 0.5f, 0.2f));
		return;
	}

	USelection* SelectedActors = GEditor ? GEditor->GetSelectedActors() : nullptr;
	if (!SelectedActors || SelectedActors->Num() == 0)
	{
		UpdateStatusText(NSLOCTEXT("OnsetStudio", "NoLevelActor", "Select a character or NPC actor in the level editor viewport first."), FLinearColor(0.9f, 0.5f, 0.2f));
		return;
	}

	int32 AppliedCount = 0;
	for (FSelectionIterator It(*SelectedActors); It; ++It)
	{
		if (AActor* Actor = Cast<AActor>(*It))
		{
			if (UOnsetAppearanceComponent* AppComp = Actor->FindComponentByClass<UOnsetAppearanceComponent>())
			{
				AppComp->ApplyRecipe(Appearance->Recipe);
				AppliedCount++;
			}
			else if (ACharacter* Char = Cast<ACharacter>(Actor))
			{
				UOnsetAppearanceComponent* NewComp = NewObject<UOnsetAppearanceComponent>(Char, TEXT("RuntimeAppearance"));
				NewComp->RegisterComponent();
				NewComp->BindExternalBodyMesh(Char->GetMesh());
				NewComp->ApplyRecipe(Appearance->Recipe);
				AppliedCount++;
			}
		}
	}

	if (AppliedCount > 0)
	{
		UpdateStatusText(FText::Format(NSLOCTEXT("OnsetStudio", "AppliedSuccess", "Applied '{0}' recipe to {1} level actor(s)."), Appearance->DisplayName, FText::AsNumber(AppliedCount)), FLinearColor(0.2f, 0.9f, 0.4f));
	}
	else
	{
		UpdateStatusText(NSLOCTEXT("OnsetStudio", "AppliedNone", "No compatible character or appearance component found on selection."), FLinearColor(0.9f, 0.3f, 0.3f));
	}
}

void UOnsetAppearanceStudioWidget::CreateNewAsset()
{
	const FString TargetDir = (CurrentMode == EOnsetAppearanceStudioMode::Appearances)
		? OnsetStudioConstants::AppearanceAssetPath
		: OnsetStudioConstants::AnimationProfileAssetPath;

	const FString BaseName = (CurrentMode == EOnsetAppearanceStudioMode::Appearances) ? TEXT("App_NewAppearance") : TEXT("AnimProfile_NewProfile");
	FString AssetName = BaseName;
	int32 Suffix = 1;

	while (FindObject<UObject>(nullptr, *(TargetDir / AssetName)) != nullptr)
	{
		AssetName = FString::Printf(TEXT("%s_%d"), *BaseName, ++Suffix);
	}

	const FString PackagePath = TargetDir / AssetName;
	UPackage* Package = CreatePackage(*PackagePath);
	Package->FullyLoad();

	UObject* NewAsset = nullptr;
	if (CurrentMode == EOnsetAppearanceStudioMode::Appearances)
	{
		UOnsetAuthoredAppearance* NewApp = NewObject<UOnsetAuthoredAppearance>(Package, *AssetName, RF_Public | RF_Standalone);
		NewApp->AppearanceId = FName(*AssetName);
		NewApp->DisplayName = FText::FromString(AssetName);
		NewApp->Category = EOnsetAppearanceCategory::NPC;
		NewApp->FactionAffiliation = TEXT("Military");
		NewApp->RankInsignia = TEXT("Civilian");
		NewAsset = NewApp;
	}
	else
	{
		UOnsetAnimationProfile* NewProf = NewObject<UOnsetAnimationProfile>(Package, *AssetName, RF_Public | RF_Standalone);
		NewProf->ProfileId = FName(*AssetName);
		NewProf->DisplayName = FText::FromString(AssetName);
		NewAsset = NewProf;
	}

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewAsset);

	TArray<UObject*> ObjectsToSave = { NewAsset };
	UPackageTools::SavePackagesForObjects(ObjectsToSave);

	RefreshAssetList();
	OnAssetSelected(FName(*AssetName), NewAsset);
	UpdateStatusText(FText::Format(NSLOCTEXT("OnsetStudio", "Created", "Created new asset '{0}'"), FText::FromString(AssetName)), FLinearColor(0.2f, 0.9f, 0.4f));
}

void UOnsetAppearanceStudioWidget::DuplicateSelectedAsset()
{
	if (!CurrentSelectedObject)
	{
		UpdateStatusText(NSLOCTEXT("OnsetStudio", "SelectToDup", "Select an asset to duplicate."), FLinearColor(0.9f, 0.6f, 0.2f));
		return;
	}

	const FString TargetDir = (CurrentMode == EOnsetAppearanceStudioMode::Appearances)
		? OnsetStudioConstants::AppearanceAssetPath
		: OnsetStudioConstants::AnimationProfileAssetPath;

	const FString SourceName = CurrentSelectedObject->GetName();
	FString NewName = SourceName + TEXT("_Variant");
	int32 Suffix = 1;

	while (FindObject<UObject>(nullptr, *(TargetDir / NewName)) != nullptr)
	{
		NewName = FString::Printf(TEXT("%s_Variant%d"), *SourceName, ++Suffix);
	}

	const FString PackagePath = TargetDir / NewName;
	UPackage* Package = CreatePackage(*PackagePath);
	Package->FullyLoad();

	UObject* Duplicated = DuplicateObject<UObject>(CurrentSelectedObject, Package, *NewName);
	Duplicated->SetFlags(RF_Public | RF_Standalone);

	if (UOnsetAuthoredAppearance* App = Cast<UOnsetAuthoredAppearance>(Duplicated))
	{
		App->AppearanceId = FName(*NewName);
		App->DisplayName = FText::FromString(NewName);
	}
	else if (UOnsetAnimationProfile* Prof = Cast<UOnsetAnimationProfile>(Duplicated))
	{
		Prof->ProfileId = FName(*NewName);
		Prof->DisplayName = FText::FromString(NewName);
	}

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Duplicated);

	TArray<UObject*> ObjectsToSave = { Duplicated };
	UPackageTools::SavePackagesForObjects(ObjectsToSave);

	RefreshAssetList();
	OnAssetSelected(FName(*NewName), Duplicated);
	UpdateStatusText(FText::Format(NSLOCTEXT("OnsetStudio", "Duplicated", "Duplicated '{0}' to '{1}'"), FText::FromString(SourceName), FText::FromString(NewName)), FLinearColor(0.2f, 0.9f, 0.4f));
}

void UOnsetAppearanceStudioWidget::DeleteSelectedAsset()
{
	if (!CurrentSelectedObject) return;

	const FString AssetName = CurrentSelectedObject->GetName();
	UPackage* Package = CurrentSelectedObject->GetOutermost();

	TArray<UObject*> ObjectsToDelete = { CurrentSelectedObject };
	TArray<UObject*> SubObjects;
	GetObjectsWithOuter(CurrentSelectedObject, SubObjects);
	ObjectsToDelete.Append(SubObjects);

	ObjectTools::ForceDeleteObjects(ObjectsToDelete, false);

	CurrentSelectedObject = nullptr;
	CurrentSelectedId = NAME_None;
	RefreshAssetList();
	UpdateStatusText(FText::Format(NSLOCTEXT("OnsetStudio", "Deleted", "Deleted asset '{0}'"), FText::FromString(AssetName)), FLinearColor(0.9f, 0.4f, 0.4f));
}

void UOnsetAppearanceStudioWidget::SaveSelectedAsset()
{
	if (!CurrentSelectedObject) return;

	CurrentSelectedObject->MarkPackageDirty();
	TArray<UObject*> ToSave = { CurrentSelectedObject };
	UPackageTools::SavePackagesForObjects(ToSave);
	UpdateStatusText(FText::Format(NSLOCTEXT("OnsetStudio", "Saved", "Saved '{0}' successfully."), FText::FromName(CurrentSelectedId)), FLinearColor(0.2f, 0.9f, 0.4f));
}

void UOnsetAppearanceStudioWidget::SaveAllAssets()
{
	TArray<UObject*> ToSave;
	if (CurrentSelectedObject && CurrentSelectedObject->GetOutermost()->IsDirty())
	{
		ToSave.Add(CurrentSelectedObject);
	}

	if (ToSave.Num() > 0)
	{
		UPackageTools::SavePackagesForObjects(ToSave);
		UpdateStatusText(NSLOCTEXT("OnsetStudio", "SavedAll", "All modified assets saved to disk."), FLinearColor(0.2f, 0.9f, 0.4f));
	}
}

void UOnsetAppearanceStudioWidget::EnsureStarterAssetsSeeded()
{
	// Ensure directories exist and seed starter animation profiles and appearances
	const FString AnimDir = OnsetStudioConstants::AnimationProfileAssetPath;
	const FString AppDir = OnsetStudioConstants::AppearanceAssetPath;

	TArray<UObject*> SeededAssets;

	auto GetOrCreatePackage = [](const FString& Path, const FString& Name) -> UPackage*
	{
		const FString FullPath = Path / Name;
		UPackage* Pkg = CreatePackage(*FullPath);
		Pkg->FullyLoad();
		return Pkg;
	};

	// 1. Military Animation Profile
	const FString MilAnimName = TEXT("AnimProfile_ConfederateMilitary");
	if (!FindObject<UOnsetAnimationProfile>(nullptr, *(AnimDir / MilAnimName)))
	{
		UPackage* Pkg = GetOrCreatePackage(AnimDir, MilAnimName);
		UOnsetAnimationProfile* Prof = NewObject<UOnsetAnimationProfile>(Pkg, *MilAnimName, RF_Public | RF_Standalone);
		Prof->ProfileId = FName(*MilAnimName);
		Prof->DisplayName = NSLOCTEXT("OnsetStudio", "MilAnimTitle", "Confederate Military (Formal / Disciplined)");
		Prof->RigCompatibility = TEXT("EpicHumanoidAdult");
		Prof->DefaultIdleAnim = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Characters/Humanoid/Adult/Anims/Unarmed/MM_Idle_F_Adult.MM_Idle_F_Adult")));
		Prof->LocomotionBlendSpace = TSoftObjectPtr<UBlendSpace>(FSoftObjectPath(TEXT("/Game/Characters/Humanoid/Adult/Anims/Unarmed/BS_Idle_Walk_Run_F_Adult.BS_Idle_Walk_Run_F_Adult")));
		Prof->SamplePreviewAnimations.Add(TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Characters/Humanoid/Adult/Anims/Unarmed/MM_Idle_F_Adult.MM_Idle_F_Adult"))));
		Prof->SamplePreviewAnimations.Add(TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Characters/Humanoid/Adult/Common/Animation/A_ACC_EpicHmn_Adult_Wait_01.A_ACC_EpicHmn_Adult_Wait_01"))));
		Prof->SamplePreviewAnimations.Add(TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Characters/Humanoid/Adult/Common/Animation/A_ACC_EpicHmn_Adult_Wait_03.A_ACC_EpicHmn_Adult_Wait_03"))));
		Prof->Notes = TEXT("Disciplined military posture, formal salute protocol, and alert stances.");
		Pkg->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(Prof);
		SeededAssets.Add(Prof);
	}

	// 2. Civilian Animation Profile
	const FString CivAnimName = TEXT("AnimProfile_CivilianCitizen");
	if (!FindObject<UOnsetAnimationProfile>(nullptr, *(AnimDir / CivAnimName)))
	{
		UPackage* Pkg = GetOrCreatePackage(AnimDir, CivAnimName);
		UOnsetAnimationProfile* Prof = NewObject<UOnsetAnimationProfile>(Pkg, *CivAnimName, RF_Public | RF_Standalone);
		Prof->ProfileId = FName(*CivAnimName);
		Prof->DisplayName = NSLOCTEXT("OnsetStudio", "CivAnimTitle", "Civilian Citizen (Relaxed / Casual)");
		Prof->RigCompatibility = TEXT("EpicHumanoidAdult");
		Prof->DefaultIdleAnim = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Characters/Humanoid/Adult/Common/Animation/A_ACC_EpicHmn_Adult_Wait_01.A_ACC_EpicHmn_Adult_Wait_01")));
		Prof->SamplePreviewAnimations.Add(TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Characters/Humanoid/Adult/Common/Animation/A_ACC_EpicHmn_Adult_Wait_01.A_ACC_EpicHmn_Adult_Wait_01"))));
		Prof->SamplePreviewAnimations.Add(TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Characters/Humanoid/Adult/Common/Animation/A_ACC_EpicHmn_Adult_Wait_03.A_ACC_EpicHmn_Adult_Wait_03"))));
		Prof->Notes = TEXT("Relaxed posture, conversational gestures, polite nods.");
		Pkg->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(Prof);
		SeededAssets.Add(Prof);
	}

	// 3. Substrate Scholar Animation Profile
	const FString SubAnimName = TEXT("AnimProfile_SubstrateScholar");
	if (!FindObject<UOnsetAnimationProfile>(nullptr, *(AnimDir / SubAnimName)))
	{
		UPackage* Pkg = GetOrCreatePackage(AnimDir, SubAnimName);
		UOnsetAnimationProfile* Prof = NewObject<UOnsetAnimationProfile>(Pkg, *SubAnimName, RF_Public | RF_Standalone);
		Prof->ProfileId = FName(*SubAnimName);
		Prof->DisplayName = NSLOCTEXT("OnsetStudio", "SubAnimTitle", "Substrate Directorate (Deliberate / Resonant)");
		Prof->RigCompatibility = TEXT("EpicHumanoidAdult");
		Prof->DefaultIdleAnim = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Characters/Humanoid/Adult/Common/Animation/A_ACC_EpicHmn_Adult_Wait_03.A_ACC_EpicHmn_Adult_Wait_03")));
		Prof->SamplePreviewAnimations.Add(TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Characters/Humanoid/Adult/Common/Animation/A_ACC_EpicHmn_Adult_Wait_03.A_ACC_EpicHmn_Adult_Wait_03"))));
		Prof->SamplePreviewAnimations.Add(TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Characters/Humanoid/Adult/Common/Animation/A_ACC_EpicHmn_Adult_Wait_04.A_ACC_EpicHmn_Adult_Wait_04"))));
		Prof->Notes = TEXT("Intellectual and resonant posture, deliberate gestures, meridian palm protocols.");
		Pkg->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(Prof);
		SeededAssets.Add(Prof);
	}

	// 4. Starter Appearance: Cadre Instructor Vane
	const FString AppVaneName = TEXT("App_AcademyCadreInstructor");
	if (!FindObject<UOnsetAuthoredAppearance>(nullptr, *(AppDir / AppVaneName)))
	{
		UPackage* Pkg = GetOrCreatePackage(AppDir, AppVaneName);
		UOnsetAuthoredAppearance* App = NewObject<UOnsetAuthoredAppearance>(Pkg, *AppVaneName, RF_Public | RF_Standalone);
		App->AppearanceId = FName(*AppVaneName);
		App->DisplayName = NSLOCTEXT("OnsetStudio", "AppVaneTitle", "Cadre Instructor (Officer Vane)");
		App->Category = EOnsetAppearanceCategory::NPC;
		App->FactionAffiliation = TEXT("Military");
		App->RankInsignia = TEXT("SeniorOfficer");
		App->Recipe.BodyId = TEXT("epic.adult.male");
		App->Recipe.HeadId = TEXT("epic_hmn_adult_M_1");
		App->Recipe.SkinColor = FLinearColor(0.78f, 0.58f, 0.42f, 1.f);
		App->Recipe.EyeColor = FLinearColor(0.12f, 0.35f, 0.55f, 1.f);
		App->AnimationProfile = TSoftObjectPtr<UOnsetAnimationProfile>(FSoftObjectPath(AnimDir / MilAnimName + TEXT(".") + MilAnimName));
		App->Tags.Add(TEXT("Academy"));
		App->Tags.Add(TEXT("Instructor"));
		App->Tags.Add(TEXT("Officer"));
		App->LoreNotes = TEXT("Senior cadre drillmaster at Confederate Starfleet Academy. Commands respect across all branches.");
		Pkg->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(App);
		SeededAssets.Add(App);
	}

	// 5. Starter Appearance: Senior Analyst Lyra
	const FString AppLyraName = TEXT("App_MeridianAnalyst");
	if (!FindObject<UOnsetAuthoredAppearance>(nullptr, *(AppDir / AppLyraName)))
	{
		UPackage* Pkg = GetOrCreatePackage(AppDir, AppLyraName);
		UOnsetAuthoredAppearance* App = NewObject<UOnsetAuthoredAppearance>(Pkg, *AppLyraName, RF_Public | RF_Standalone);
		App->AppearanceId = FName(*AppLyraName);
		App->DisplayName = NSLOCTEXT("OnsetStudio", "AppLyraTitle", "Senior Analyst Lyra");
		App->Category = EOnsetAppearanceCategory::NPC;
		App->FactionAffiliation = TEXT("Substrate");
		App->RankInsignia = TEXT("SeniorAnalyst");
		App->Recipe.BodyId = TEXT("epic.adult.female");
		App->Recipe.HeadId = TEXT("epic_hmn_adult_F_1");
		App->Recipe.SkinColor = FLinearColor(0.85f, 0.65f, 0.52f, 1.f);
		App->Recipe.EyeColor = FLinearColor(0.2f, 0.7f, 0.6f, 1.f);
		App->AnimationProfile = TSoftObjectPtr<UOnsetAnimationProfile>(FSoftObjectPath(AnimDir / SubAnimName + TEXT(".") + SubAnimName));
		App->Tags.Add(TEXT("Substrate"));
		App->Tags.Add(TEXT("Analyst"));
		App->LoreNotes = TEXT("Meridian Institute Substrate resonance specialist. Evaluates anomalous field emissions.");
		Pkg->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(App);
		SeededAssets.Add(App);
	}

	// 6. Starter Appearance: Quartermaster Bren
	const FString AppBrenName = TEXT("App_AegisQuartermaster");
	if (!FindObject<UOnsetAuthoredAppearance>(nullptr, *(AppDir / AppBrenName)))
	{
		UPackage* Pkg = GetOrCreatePackage(AppDir, AppBrenName);
		UOnsetAuthoredAppearance* App = NewObject<UOnsetAuthoredAppearance>(Pkg, *AppBrenName, RF_Public | RF_Standalone);
		App->AppearanceId = FName(*AppBrenName);
		App->DisplayName = NSLOCTEXT("OnsetStudio", "AppBrenTitle", "Quartermaster Bren");
		App->Category = EOnsetAppearanceCategory::NPC;
		App->FactionAffiliation = TEXT("Logistics");
		App->RankInsignia = TEXT("TribuneOverseer");
		App->Recipe.BodyId = TEXT("epic.adult.male");
		App->Recipe.HeadId = TEXT("epic_hmn_adult_M_1");
		App->Recipe.SkinColor = FLinearColor(0.72f, 0.52f, 0.38f, 1.f);
		App->AnimationProfile = TSoftObjectPtr<UOnsetAnimationProfile>(FSoftObjectPath(AnimDir / MilAnimName + TEXT(".") + MilAnimName));
		App->Tags.Add(TEXT("Logistics"));
		App->Tags.Add(TEXT("Armory"));
		App->LoreNotes = TEXT("House Aegis Ordnance Quartermaster. Manages field requisitions and weapons stockpiles.");
		Pkg->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(App);
		SeededAssets.Add(App);
	}

	if (SeededAssets.Num() > 0)
	{
		UPackageTools::SavePackagesForObjects(SeededAssets);
	}
}
