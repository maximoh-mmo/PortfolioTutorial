// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/OnsetEnemyEditorWidget.h"

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
#include "OnsetEditor.h"
#include "PackageTools.h"
#include "Subsystem/OnsetPoolSubsystem.h"
#include "UI/OnsetAbilityRowButton.h"
#include "UI/OnsetEnemyCreationDialog.h"
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

/** Generates a unique widget name so WidgetTree names never collide across rows. */
static FName EnemyNextWidgetName(const FString& Base)
{
	static int32 WidgetCounter = 0;
	return FName(*FString::Printf(TEXT("%s_%d"), *Base, ++WidgetCounter));
}

/** Maximum row-name length; keeps generated names well under FName's truncation limit. */
constexpr int32 MaxEnemyCleanNameLength = 60;

/**
 * Sanitizes a display name into a valid DataTable row name. Same rules as the
 * ability editor: alphanumerics kept, separator runs collapse to a single '_'.
 */
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

/** Flat, compact button style: no chrome, subtle hover/press fill. */
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

/** Row style: transparent normally; tinted fill when the row is selected. */
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

/** A fixed-width text cell (used for both the column header and row values). */
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

/** Creates a compact UButton whose content is a single centered text block. */
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

	// --- Bottom: full-width action bar ---
	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonRow"));

	UButton* AddButton = MakeEnemyTextButton(WidgetTree, NSLOCTEXT("OnsetEnemyEditor", "Add", "Add"), TEXT("Add"));
	AddButton->OnClicked.AddDynamic(this, &UOnsetEnemyEditorWidget::AddDefinition);
	ButtonRow->AddChildToHorizontalBox(AddButton)->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));

	UButton* DeleteButton = MakeEnemyTextButton(WidgetTree, NSLOCTEXT("OnsetEnemyEditor", "Delete", "Delete"), TEXT("Delete"));
	DeleteButton->OnClicked.AddDynamic(this, &UOnsetEnemyEditorWidget::DeleteDefinition);
	ButtonRow->AddChildToHorizontalBox(DeleteButton)->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));

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
		FName RowName = RowPair.Key;
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
		const FText XpText = FText::AsNumber(Stats->XpReward);
		const FText TierText = FText::AsNumber(1); // Default preview tier; real tiers live on the spawner.

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
}

void UOnsetEnemyEditorWidget::SelectRow(FName RowName)
{
	// Commit any pending edits of the previously selected row before switching.
	WriteBackRow();

	UDataTable* Table = GetEnemyStatsTable();
	if (!Table)
	{
		return;
	}

	const FOnsetEnemyStats* Stats = Table->FindRow<FOnsetEnemyStats>(RowName, nullptr);
	if (!Stats)
	{
		return;
	}

	SelectedRowName = RowName;

	if (!EditWrapper)
	{
		EditWrapper = NewObject<UOnsetEnemyEditRowWrapper>(this);
	}
	EditWrapper->Stats = *Stats;

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
		return;
	}

	const FOnsetEnemyStats& Stats = EditWrapper->Stats;
	const int32 EnemyLevel = FMath::Max(1, Stats.Level);
	const int32 BaseXp = UOnsetLevelingLibrary::GetEnemyBaseXP(EnemyLevel, Stats.XpReward);
	const int32 Required = UOnsetLevelingLibrary::GetXPRequired(EnemyLevel);
	const int32 OnLevelGrant = UOnsetLevelingLibrary::GetGrantedXP(EnemyLevel, EnemyLevel, Stats.XpReward);

	const FText Text = FText::Format(
		NSLOCTEXT("OnsetEnemyEditor", "XpPreview", "XP at level {0}: required {1} | base per kill {2} | on-level grant {3} | kills/level {4:.1f}"),
		EnemyLevel, Required, BaseXp, OnLevelGrant, UOnsetLevelingLibrary::GetKillsPerLevel());
	PreviewText->SetText(Text);
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
		.ClientSize(FVector2D(360.0f, 260.0f))
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

	UOnsetEnemyCreationData* Data = PendingCreationData;

	const FString Clean = CleanEnemyName(Data->DisplayName.ToString());
	FName NewRowName = FName(*Clean);
	int32 Suffix = 1;
	while (Table->FindRow<FOnsetEnemyStats>(NewRowName, nullptr))
	{
		NewRowName = FName(*FString::Printf(TEXT("%s%d"), *Clean, Suffix++));
	}

	FOnsetEnemyStats NewStats;
	NewStats.DisplayName = Data->DisplayName;
	NewStats.Level = 1;
	NewStats.MaxHealth = 100.0f;
	NewStats.DamageBase = 25.0f;
	NewStats.Defense = 10.0f;
	NewStats.Luck = 10.0f;

	if (Data->bAutoCreateProfiles)
	{
		const FString BaseName = NewRowName.ToString();
		const FString ProfileFolder = TEXT("/Game/AI");

		// Visual profile.
		const FString VPPath = FString::Printf(TEXT("%s/%s_VP"), *ProfileFolder, *BaseName);
		if (UPackage* Package = CreatePackage(*VPPath))
		{
			UVisualProfile* VP = NewObject<UVisualProfile>(Package, UVisualProfile::StaticClass(), FName(*FString::Printf(TEXT("%s_VP"), *BaseName)), RF_Public | RF_Standalone);
			NewStats.VisualProfile = VP;
		}

		// AI profile.
		const FString AIPath = FString::Printf(TEXT("%s/%s_AI"), *ProfileFolder, *BaseName);
		if (UPackage* Package = CreatePackage(*AIPath))
		{
			UAIProfile* AI = NewObject<UAIProfile>(Package, UAIProfile::StaticClass(), FName(*FString::Printf(TEXT("%s_AI"), *BaseName)), RF_Public | RF_Standalone);
			NewStats.AIProfile = AI;
		}

		// Perception profile.
		const FString PPPath = FString::Printf(TEXT("%s/%s_PP"), *ProfileFolder, *BaseName);
		if (UPackage* Package = CreatePackage(*PPPath))
		{
			UPerceptionProfile* PP = NewObject<UPerceptionProfile>(Package, UPerceptionProfile::StaticClass(), FName(*FString::Printf(TEXT("%s_PP"), *BaseName)), RF_Public | RF_Standalone);
			NewStats.PerceptionProfile = PP;
		}
	}

	Table->AddRow(NewRowName, NewStats);

	// Persist later when the editor closes (see PersistOnClose).
	bDirty = true;

	PendingCreationData = nullptr;

	RefreshFromTable();
	SelectRow(NewRowName);
}

void UOnsetEnemyEditorWidget::DeleteDefinition()
{
	if (SelectedRowName.IsNone() || !CachedTable)
	{
		return;
	}

	CachedTable->RemoveRow(SelectedRowName);
	SelectedRowName = NAME_None;
	if (PropertyView)
	{
		PropertyView->SetObject(nullptr);
	}
	if (PreviewText)
	{
		PreviewText->SetText(FText::GetEmpty());
	}

	// Persist later when the editor closes (see PersistOnClose).
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
	// The user changed a property in the details panel: write the new value into
	// the in-memory table immediately so switching rows never loses the edit.
	WriteBackRow();
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

	// Commit any pending edit of the currently selected row.
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

	// Write back any in-progress edits so the spawned enemy reflects them.
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

	// Spawn ~300 units in front of the camera so it's visible on-screen.
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