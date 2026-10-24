#include "UI/OnsetEnemyEditorWidget.h"
#include "Editor.h"
#include "Engine/Engine.h"

#include "AI/OnsetAIController.h"
#include "Combat/OnsetEquipmentLibrary.h"
#include "Combat/OnsetLevelingLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/DetailsView.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Enemy/OnsetEnemy.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "Enemy/Profile/AIProfile.h"
#include "Enemy/Profile/PerceptionProfile.h"
#include "Enemy/Profile/VisualProfile.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "LevelEditorViewport.h"
#include "OnsetAppearancePreviewActor.h"
#include "OnsetAuthoredAppearance.h"
#include "OnsetEditor.h"
#include "PackageTools.h"
#include "Selection.h"
#include "Subsystem/OnsetPoolSubsystem.h"
#include "UI/OnsetAbilityRowButton.h"
#include "UI/OnsetEnemyCreationDialog.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UI/OnsetNotifyDetailsView.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/SWindow.h"

namespace OnsetEnemyEditorWidgetConstants
{
	constexpr float ListEntryHeight = 22.0f;
	constexpr float NameWidth = 160.0f;
	constexpr float LevelWidth = 60.0f;
	constexpr float XpWidth = 80.0f;
	constexpr float TierWidth = 70.0f;
}

static FName EnemyNextWidgetName(const FString& Base)
{
	static int32 WidgetCounter = 0;
	return FName(*FString::Printf(TEXT("%s_%d"), *Base, ++WidgetCounter));
}

constexpr int32 MaxEnemyCleanNameLength = 60;

static FString CleanEnemyName(const FString& RawName)
{
	const FString Trimmed = RawName.TrimStartAndEnd();
	FString Clean;
	Clean.Reserve(FMath::Min(Trimmed.Len(), MaxEnemyCleanNameLength));

	for (const TCHAR Char : Trimmed)
	{
		if (Clean.Len() >= MaxEnemyCleanNameLength)
		{
			break;
		}

		if (FChar::IsAlnum(Char))
		{
			Clean.AppendChar(Char);
		}
		else if (!Clean.IsEmpty() && Clean[Clean.Len() - 1] != TEXT('_'))
		{
			Clean.AppendChar(TEXT('_'));
		}
	}

	while (Clean.Len() > 0 && Clean[Clean.Len() - 1] == TEXT('_'))
	{
		Clean.RemoveAt(Clean.Len() - 1);
	}

	if (Clean.IsEmpty())
	{
		Clean = TEXT("Enemy");
	}

	return Clean;
}

static FButtonStyle MakeEnemyFlatStyle()
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

static FButtonStyle MakeEnemyRowStyle(bool bSelected)
{
	FButtonStyle Style = MakeEnemyFlatStyle();
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

static USizeBox* MakeEnemyCell(UWidgetTree* WidgetTree, const FString& BaseName, float Width, const FText& Text, bool bHeader, ETextJustify::Type Justification)
{
	USizeBox* CellBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), EnemyNextWidgetName(BaseName + TEXT("_Cell")));
	CellBox->SetWidthOverride(Width);

	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), EnemyNextWidgetName(BaseName + TEXT("_Label")));
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

static UButton* MakeEnemyTextButton(UWidgetTree* WidgetTree, const FText& Label, const FString& BaseName)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), EnemyNextWidgetName(BaseName + TEXT("_Button")));
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), EnemyNextWidgetName(BaseName + TEXT("_Label")));
	Text->SetText(Label);
	Text->SetJustification(ETextJustify::Center);
	Text->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10));
	Button->SetContent(Text);
	Button->SetStyle(MakeEnemyFlatStyle());
	return Button;
}

UDataTable* UOnsetEnemyEditorWidget::GetEnemyStatsTable()
{
	if (!CachedTable)
	{
		CachedTable = UOnsetEquipmentLibrary::GetEnemyStatsTable();
	}
	return CachedTable;
}

void UOnsetEnemyEditorWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildUI();
	RefreshFromTable();
}

void UOnsetEnemyEditorWidget::NativeDestruct()
{
	CleanupPreviewActor();
	PersistOnClose();
	Super::NativeDestruct();
}

void UOnsetEnemyEditorWidget::OpenEditor()
{
	BuildUI();
	RefreshFromTable();
}

void UOnsetEnemyEditorWidget::BuildUI()
{
	if (WidgetTree && WidgetTree->RootWidget)
	{
		return;
	}

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootLayout"));
	UHorizontalBox* ContentRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ContentRow"));

	// --- Left: column header + scrollable enemy list ---
	UVerticalBox* LeftPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftPanel"));

	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ColumnHeader"));
	HeaderRow->AddChild(MakeEnemyCell(WidgetTree, TEXT("HeaderName"), OnsetEnemyEditorWidgetConstants::NameWidth, NSLOCTEXT("OnsetEnemyEditor", "ColumnName", "Name"), true, ETextJustify::Left));
	HeaderRow->AddChild(MakeEnemyCell(WidgetTree, TEXT("HeaderLevel"), OnsetEnemyEditorWidgetConstants::LevelWidth, NSLOCTEXT("OnsetEnemyEditor", "ColumnLevel", "Level"), true, ETextJustify::Center));
	HeaderRow->AddChild(MakeEnemyCell(WidgetTree, TEXT("HeaderXp"), OnsetEnemyEditorWidgetConstants::XpWidth, NSLOCTEXT("OnsetEnemyEditor", "ColumnXp", "XP"), true, ETextJustify::Center));
	HeaderRow->AddChild(MakeEnemyCell(WidgetTree, TEXT("HeaderTier"), OnsetEnemyEditorWidgetConstants::TierWidth, NSLOCTEXT("OnsetEnemyEditor", "ColumnTier", "Tier"), true, ETextJustify::Center));
	LeftPanel->AddChildToVerticalBox(HeaderRow);

	UScrollBox* ListScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RowList"));
	ListScroll->SetAlwaysShowScrollbar(true);
	ListScroll->SetAlwaysShowScrollbarTrack(true);
	LeftPanel->AddChildToVerticalBox(ListScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	RowListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RowListBox"));
	ListScroll->AddChild(RowListBox);

	// --- Right: property view bound to the selected row ---
	PropertyView = WidgetTree->ConstructWidget<UOnsetNotifyDetailsView>(UOnsetNotifyDetailsView::StaticClass(), TEXT("PropertyView"));
	PropertyView->OnPropertyEdited.AddUObject(this, &UOnsetEnemyEditorWidget::HandlePropertyEdited);

	ContentRow->AddChildToHorizontalBox(LeftPanel)->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	ContentRow->AddChildToHorizontalBox(PropertyView)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	RootBox->AddChildToVerticalBox(ContentRow)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// --- XP preview line under the details panel ---
	PreviewText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PreviewText"));
	PreviewText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10));
	PreviewText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.8f, 0.7f, 1.0f)));
	RootBox->AddChildToVerticalBox(PreviewText)->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));

	// --- Multi-Tier Combat Scaling Matrix ---
	ScalingMatrixText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ScalingMatrixText"));
	ScalingMatrixText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 9));
	ScalingMatrixText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.95f, 1.0f)));
	RootBox->AddChildToVerticalBox(ScalingMatrixText)->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));

	// --- Bottom: full-width action bar ---
	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonRow"));

	UButton* AddButton = MakeEnemyTextButton(WidgetTree, NSLOCTEXT("OnsetEnemyEditor", "Add", "Add"), TEXT("Add"));
	AddButton->OnClicked.AddDynamic(this, &UOnsetEnemyEditorWidget::AddDefinition);
	ButtonRow->AddChildToHorizontalBox(AddButton)->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));

	UButton* DeleteButton = MakeEnemyTextButton(WidgetTree, NSLOCTEXT("OnsetEnemyEditor", "Delete", "Delete"), TEXT("Delete"));
	DeleteButton->OnClicked.AddDynamic(this, &UOnsetEnemyEditorWidget::DeleteDefinition);
	ButtonRow->AddChildToHorizontalBox(DeleteButton)->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));

	UButton* PreviewAppBtn = MakeEnemyTextButton(WidgetTree, NSLOCTEXT("OnsetEnemyEditor", "PreviewApp", "Preview In Viewport"), TEXT("PreviewApp"));
	PreviewAppBtn->OnClicked.AddDynamic(this, &UOnsetEnemyEditorWidget::PreviewAppearanceInViewport);
	ButtonRow->AddChildToHorizontalBox(PreviewAppBtn)->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));

	UButton* TestButton = MakeEnemyTextButton(WidgetTree, NSLOCTEXT("OnsetEnemyEditor", "TestInPIE", "Test in PIE"), TEXT("Test"));
	TestButton->OnClicked.AddDynamic(this, &UOnsetEnemyEditorWidget::TestInPIE);
	ButtonRow->AddChildToHorizontalBox(TestButton);

	UVerticalBoxSlot* ButtonRowSlot = RootBox->AddChildToVerticalBox(ButtonRow);
	ButtonRowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	ButtonRowSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
	ButtonRowSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));

	WidgetTree->RootWidget = RootBox;
}

void UOnsetEnemyEditorWidget::RefreshFromTable()
{
	RebuildList();
	UpdatePreviewText();
}

void UOnsetEnemyEditorWidget::RebuildList()
{
	if (!RowListBox)
	{
		return;
	}

	RowListBox->ClearChildren();
	RowButtons.Reset();

	UDataTable* Table = GetEnemyStatsTable();
	if (!Table)
	{
		return;
	}

	for (const TPair<FName, uint8*>& RowPair : Table->GetRowMap())
	{
		const FName RowName = RowPair.Key;
		const FOnsetEnemyStats* Stats = reinterpret_cast<const FOnsetEnemyStats*>(RowPair.Value);
		if (!Stats)
		{
			continue;
		}

		UOnsetAbilityRowButton* RowButton = NewObject<UOnsetAbilityRowButton>(this);
		RowButton->RowName = RowName;
		RowButton->SetStyle(MakeEnemyRowStyle(RowName == SelectedRowName));

		USizeBox* RowHeightBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), EnemyNextWidgetName(TEXT("RowSize")));
		RowHeightBox->SetHeightOverride(OnsetEnemyEditorWidgetConstants::ListEntryHeight);

		UHorizontalBox* Cells = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), EnemyNextWidgetName(TEXT("RowCells")));

		const FText NameText = Stats->DisplayName.IsEmpty() ? FText::FromName(RowName) : Stats->DisplayName;
		const FText LevelText = FText::AsNumber(Stats->Level);
		const int32 BaseXp = UOnsetLevelingLibrary::GetEnemyBaseXP(FMath::Max(1, Stats->Level), Stats->XpReward);
		const FText XpText = FText::AsNumber(BaseXp);

		FText TierText;
		if (Stats->VisualProfile && !Stats->VisualProfile->SkeletalMesh.IsNull())
		{
			TierText = NSLOCTEXT("OnsetEnemyEditor", "ProfileReady", "Profile");
		}
		else if (!Stats->AuthoredAppearance.IsNull())
		{
			TierText = NSLOCTEXT("OnsetEnemyEditor", "AppearanceReady", "Modular");
		}
		else
		{
			TierText = FText::FromString(TEXT("—"));
		}

		Cells->AddChild(MakeEnemyCell(WidgetTree, TEXT("Name"), OnsetEnemyEditorWidgetConstants::NameWidth, NameText, false, ETextJustify::Left));
		Cells->AddChild(MakeEnemyCell(WidgetTree, TEXT("Level"), OnsetEnemyEditorWidgetConstants::LevelWidth, LevelText, false, ETextJustify::Center));
		Cells->AddChild(MakeEnemyCell(WidgetTree, TEXT("Xp"), OnsetEnemyEditorWidgetConstants::XpWidth, XpText, false, ETextJustify::Center));
		Cells->AddChild(MakeEnemyCell(WidgetTree, TEXT("Tier"), OnsetEnemyEditorWidgetConstants::TierWidth, TierText, false, ETextJustify::Center));

		RowHeightBox->AddChild(Cells);
		RowButton->SetContent(RowHeightBox);

		RowButton->OnClicked.AddDynamic(RowButton, &UOnsetAbilityRowButton::HandleClicked);
		RowButton->OnRowSelected.AddUObject(this, &UOnsetEnemyEditorWidget::SelectRow);

		RowListBox->AddChildToVerticalBox(RowButton)->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Left);
		RowButtons.Add(RowName, RowButton);
	}

	if (SelectedRowName.IsNone())
	{
		for (const TPair<FName, uint8*>& RowPair : Table->GetRowMap())
		{
			SelectRow(RowPair.Key);
			break;
		}
	}
}

void UOnsetEnemyEditorWidget::SelectRow(FName RowName)
{
	UDataTable* Table = GetEnemyStatsTable();
	if (!Table)
	{
		return;
	}

	const FOnsetEnemyStats* Row = Table->FindRow<FOnsetEnemyStats>(RowName, nullptr);
	if (!Row)
	{
		return;
	}

	SelectedRowName = RowName;

	if (!EditWrapper)
	{
		EditWrapper = NewObject<UOnsetEnemyEditRowWrapper>(this);
	}
	EditWrapper->Stats = *Row;

	if (PropertyView)
	{
		PropertyView->SetObject(EditWrapper);
	}

	for (const TPair<FName, TObjectPtr<UOnsetAbilityRowButton>>& Pair : RowButtons)
	{
		if (Pair.Value)
		{
			Pair.Value->SetStyle(MakeEnemyRowStyle(Pair.Key == SelectedRowName));
		}
	}

	UpdatePreviewText();
}

void UOnsetEnemyEditorWidget::UpdatePreviewText()
{
	if (!PreviewText)
	{
		return;
	}

	if (SelectedRowName.IsNone() || !CachedTable || !EditWrapper)
	{
		PreviewText->SetText(FText::GetEmpty());
		if (ScalingMatrixText) ScalingMatrixText->SetText(FText::GetEmpty());
		return;
	}

	const FOnsetEnemyStats& Stats = EditWrapper->Stats;
	const int32 EnemyLevel = FMath::Max(1, Stats.Level);
	const int32 BaseXp = UOnsetLevelingLibrary::GetEnemyBaseXP(EnemyLevel, Stats.XpReward);
	const int32 Required = UOnsetLevelingLibrary::GetXPRequired(EnemyLevel);
	const int32 OnLevelGrant = UOnsetLevelingLibrary::GetGrantedXP(EnemyLevel, EnemyLevel, Stats.XpReward);

	const FText Text = FText::Format(
		NSLOCTEXT("OnsetEnemyEditor", "XpPreview", "Level {0} XP Pacing: Required to level: {1} | Base/Kill: {2} | Grant: {3} | Kills/Level: {4:.1f}"),
		FText::AsNumber(EnemyLevel),
		FText::AsNumber(Required),
		FText::AsNumber(BaseXp),
		FText::AsNumber(OnLevelGrant),
		(OnLevelGrant > 0) ? ((double)Required / (double)OnLevelGrant) : 0.0);
	PreviewText->SetText(Text);

	if (ScalingMatrixText)
	{
		const float KDef = UOnsetEquipmentLibrary::GetKDefense();
		const float ArchetypeCooldown = FMath::Max(0.2f, UOnsetEquipmentLibrary::GetArchetypeBaseCooldown(Stats.WeaponArchetype));
		const float BaselinePlayerDps = 35.0f;

		FString MatrixStr = TEXT("=== Multi-Tier Combat Scaling Matrix ===\n");
		for (int32 Tier = 0; Tier <= 3; ++Tier)
		{
			const float DiffMult = UOnsetEquipmentLibrary::GetEnemyDifficultyMultiplier(Tier);
			const float TierHP = Stats.MaxHealth * DiffMult;
			const float TierDmg = Stats.DamageBase * DiffMult;
			const float TierEHP = TierHP * (1.0f + (Stats.Defense / KDef));
			const float TierDPS = TierDmg / ArchetypeCooldown;
			const float PlayerTTK = TierEHP / BaselinePlayerDps;
			const float EnemyTTK = (TierDPS > 0.f) ? (100.0f / TierDPS) : 999.0f;

			MatrixStr += FString::Printf(
				TEXT("Tier %d: HP %6.1f | Atk %5.1f | EHP %6.1f | DPS %5.1f | Kill Player in: %4.1fs | Die to Player in: %4.1fs\n"),
				Tier, TierHP, TierDmg, TierEHP, TierDPS, EnemyTTK, PlayerTTK);
		}

		const float T0_EnemyDPS = Stats.DamageBase / ArchetypeCooldown;
		const float T0_EnemyTTK = (T0_EnemyDPS > 0.f) ? (100.0f / T0_EnemyDPS) : 999.f;
		const float T0_PlayerTTK = (Stats.MaxHealth * (1.0f + (Stats.Defense / KDef))) / BaselinePlayerDps;

		FString PacingStatus;
		if (T0_EnemyTTK <= 2.0f)
		{
			PacingStatus = TEXT("⚠️ HAZARD: Kills standard on-level player in under 2 seconds! Reduce DamageBase or increase attack cooldown.");
			ScalingMatrixText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.4f, 0.2f)));
		}
		else if (T0_PlayerTTK >= 30.0f)
		{
			PacingStatus = TEXT("⚠️ PACING WARNING: Takes over 30s to defeat! Consider reducing MaxHealth or Defense.");
			ScalingMatrixText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.7f, 0.2f)));
		}
		else
		{
			PacingStatus = TEXT("✅ BALANCED: Combat encounter pacing within standard RPG engagement curve (3s - 12s).");
			ScalingMatrixText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.95f, 0.7f)));
		}

		MatrixStr += FString::Printf(TEXT("Status: %s"), *PacingStatus);
		ScalingMatrixText->SetText(FText::FromString(MatrixStr));
	}
}

void UOnsetEnemyEditorWidget::AddDefinition()
{
	UDataTable* Table = GetEnemyStatsTable();
	if (!Table)
	{
		return;
	}

	PendingCreationData = NewObject<UOnsetEnemyCreationData>(this, NAME_None, RF_Transient);
	PendingCreationData->DisplayName = FText::FromString(TEXT("NewEnemy"));

	TSharedRef<SEnemyCreationDialog> Dialog = SNew(SEnemyCreationDialog, PendingCreationData);

	TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
	if (!ParentWindow.IsValid())
	{
		ParentWindow = FGlobalTabmanager::Get()->GetRootWindow();
	}

	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(NSLOCTEXT("OnsetEnemyEditor", "CreateEnemy", "Create Enemy"))
		.ClientSize(FVector2D(380.0f, 220.0f))
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	Dialog->SetOwnerWindow(Window);
	Window->SetContent(Dialog);

	FSlateApplication::Get().AddModalWindow(Window, ParentWindow);

	if (!Dialog->ShouldCreate() || !PendingCreationData)
	{
		PendingCreationData = nullptr;
		return;
	}

	const FString CleanName = CleanEnemyName(PendingCreationData->DisplayName.ToString());
	const bool bAutoCreate = PendingCreationData->bAutoCreateProfiles;
	PendingCreationData = nullptr;

	FString UniqueRowName = CleanName;
	int32 Suffix = 1;
	while (Table->FindRow<FOnsetEnemyStats>(FName(*UniqueRowName), nullptr))
	{
		UniqueRowName = FString::Printf(TEXT("%s_%d"), *CleanName, ++Suffix);
	}
	const FName NewRowName(*UniqueRowName);

	FOnsetEnemyStats NewRow;
	NewRow.DisplayName = FText::FromString(CleanName);
	NewRow.Level = 1;
	NewRow.XpReward = 0;
	NewRow.MaxHealth = 100.0f;
	NewRow.DamageBase = 25.0f;
	NewRow.Defense = 10.0f;

	TArray<UObject*> NewlyCreatedProfiles;

	if (bAutoCreate)
	{
		const FString PackageBase = FString::Printf(TEXT("/Game/Data/Enemies/%s/"), *CleanName);

		auto CreateProfileAsset = [&](UClass* Class, const FString& Prefix) -> UObject*
		{
			const FString AssetName = Prefix + TEXT("_") + CleanName;
			const FString FullPath = PackageBase + AssetName;
			UPackage* Pkg = CreatePackage(*FullPath);
			Pkg->FullyLoad();
			UObject* NewAsset = NewObject<UObject>(Pkg, Class, *AssetName, RF_Public | RF_Standalone);
			Pkg->MarkPackageDirty();
			FAssetRegistryModule::AssetCreated(NewAsset);
			return NewAsset;
		};

		UVisualProfile* VP = Cast<UVisualProfile>(CreateProfileAsset(UVisualProfile::StaticClass(), TEXT("VP")));
		UAIProfile* AIP = Cast<UAIProfile>(CreateProfileAsset(UAIProfile::StaticClass(), TEXT("AI")));
		UPerceptionProfile* PP = Cast<UPerceptionProfile>(CreateProfileAsset(UPerceptionProfile::StaticClass(), TEXT("PP")));

		NewRow.VisualProfile = VP;
		NewRow.AIProfile = AIP;
		NewRow.PerceptionProfile = PP;

		NewlyCreatedProfiles.Add(VP);
		NewlyCreatedProfiles.Add(AIP);
		NewlyCreatedProfiles.Add(PP);
	}

	Table->AddRow(NewRowName, NewRow);
	Table->GetPackage()->MarkPackageDirty();

	if (NewlyCreatedProfiles.Num() > 0)
	{
		SaveTableAndProfiles(NewlyCreatedProfiles);
	}

	bDirty = true;
	RebuildList();
	SelectRow(NewRowName);
}

void UOnsetEnemyEditorWidget::DeleteDefinition()
{
	if (SelectedRowName.IsNone() || !CachedTable)
	{
		return;
	}

	CachedTable->RemoveRow(SelectedRowName);
	CachedTable->GetPackage()->MarkPackageDirty();

	SelectedRowName = NAME_None;
	if (EditWrapper)
	{
		EditWrapper->Stats = FOnsetEnemyStats();
	}
	if (PropertyView)
	{
		PropertyView->SetObject(nullptr);
	}

	bDirty = true;
	RefreshFromTable();
}

bool UOnsetEnemyEditorWidget::WriteBackRow()
{
	if (!CachedTable || SelectedRowName.IsNone() || !EditWrapper)
	{
		return false;
	}

	FOnsetEnemyStats* Stats = CachedTable->FindRow<FOnsetEnemyStats>(SelectedRowName, nullptr);
	if (!Stats)
	{
		return false;
	}

	*Stats = EditWrapper->Stats;
	return true;
}

void UOnsetEnemyEditorWidget::SaveTableAndProfiles(const TArray<UObject*>& ObjectsToSave)
{
	for (UObject* Obj : ObjectsToSave)
	{
		if (Obj)
		{
			Obj->GetPackage()->MarkPackageDirty();
		}
	}
	UPackageTools::SavePackagesForObjects(ObjectsToSave);
}

void UOnsetEnemyEditorWidget::HandlePropertyEdited()
{
	WriteBackRow();
	UpdatePreviewText();
	bDirty = true;
}

void UOnsetEnemyEditorWidget::PersistOnClose()
{
	if (!bDirty)
	{
		return;
	}

	UDataTable* Table = GetEnemyStatsTable();
	if (!Table)
	{
		return;
	}

	WriteBackRow();

	TArray<UObject*> ToSave{ Table };
	for (const TPair<FName, uint8*>& RowPair : Table->GetRowMap())
	{
		const FOnsetEnemyStats* Stats = reinterpret_cast<const FOnsetEnemyStats*>(RowPair.Value);
		if (!Stats)
		{
			continue;
		}
		if (Stats->VisualProfile) ToSave.AddUnique(Stats->VisualProfile);
		if (Stats->AIProfile) ToSave.AddUnique(Stats->AIProfile);
		if (Stats->PerceptionProfile) ToSave.AddUnique(Stats->PerceptionProfile);
	}

	SaveTableAndProfiles(ToSave);
	bDirty = false;
}

void UOnsetEnemyEditorWidget::PreviewAppearanceInViewport()
{
	if (SelectedRowName.IsNone() || !CachedTable)
	{
		return;
	}

	FOnsetEnemyStats* Stats = CachedTable->FindRow<FOnsetEnemyStats>(SelectedRowName, nullptr);
	if (!Stats)
	{
		return;
	}

	WriteBackRow();

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		return;
	}

	AOnsetAppearancePreviewActor* PreviewActor = CachedPreviewActor.Get();
	if (!PreviewActor)
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(World, AOnsetAppearancePreviewActor::StaticClass(), FoundActors);
		if (FoundActors.Num() > 0)
		{
			PreviewActor = Cast<AOnsetAppearancePreviewActor>(FoundActors[0]);
		}
		else
		{
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

			PreviewActor = World->SpawnActor<AOnsetAppearancePreviewActor>(AOnsetAppearancePreviewActor::StaticClass(), SpawnLoc, SpawnRot, SpawnParams);
		}
		CachedPreviewActor = PreviewActor;
	}

	if (!PreviewActor) return;

	if (!Stats->AuthoredAppearance.IsNull())
	{
		if (UOnsetAuthoredAppearance* Authored = Stats->AuthoredAppearance.LoadSynchronous())
		{
			PreviewActor->LoadAuthoredAppearance(Authored);
		}
	}
	else
	{
		PreviewActor->RebuildPreview();
	}

	if (GEditor)
	{
		GEditor->SelectNone(false, true);
		GEditor->SelectActor(PreviewActor, true, true);
		GEditor->MoveViewportCamerasToActor(*PreviewActor, false);
	}
}

void UOnsetEnemyEditorWidget::CleanupPreviewActor()
{
	if (CachedPreviewActor.IsValid())
	{
		CachedPreviewActor->Destroy();
		CachedPreviewActor.Reset();
	}
}

void UOnsetEnemyEditorWidget::TestInPIE()
{
	if (SelectedRowName.IsNone() || !CachedTable)
	{
		return;
	}

	FOnsetEnemyStats* Stats = CachedTable->FindRow<FOnsetEnemyStats>(SelectedRowName, nullptr);
	if (!Stats)
	{
		return;
	}

	WriteBackRow();

	UWorld* World = GEditor ? GEditor->GetPIEWorldContext() ? GEditor->GetPIEWorldContext()->World() : nullptr : nullptr;
	if (!World)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("Enemy Editor: start a PIE session first (the Test button spawns in the running PIE world)."));
		}
		return;
	}

	APlayerController* PC = GEditor->GetFirstLocalPlayerController(World);
	APawn* CameraPawn = PC ? PC->GetPawn() : nullptr;
	if (!CameraPawn)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("Enemy Editor: no local player pawn in the PIE world to spawn near."));
		}
		return;
	}

	const FVector SpawnLocation = CameraPawn->GetActorLocation() + CameraPawn->GetActorForwardVector() * 300.0f + FVector::UpVector * 100.0f;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AOnsetEnemy* Enemy = World->SpawnActor<AOnsetEnemy>(AOnsetEnemy::StaticClass(), SpawnLocation, CameraPawn->GetActorRotation(), Params);
	if (!Enemy)
	{
		return;
	}

	Enemy->ApplyProfile(Stats->VisualProfile);
	Enemy->ApplyEnemyStats(SelectedRowName, 0);
	Enemy->ZoneTag = FGameplayTag();

	AOnsetAIController* AIController = World->SpawnActor<AOnsetAIController>(AOnsetAIController::StaticClass(), SpawnLocation, CameraPawn->GetActorRotation(), Params);
	if (AIController)
	{
		AIController->ApplyAIProfile(Stats->AIProfile);
		AIController->ApplyPerceptionProfile(Stats->PerceptionProfile);
		AIController->Possess(Enemy);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Spawned enemy '%s' for testing."), *SelectedRowName.ToString()));
	}
}
