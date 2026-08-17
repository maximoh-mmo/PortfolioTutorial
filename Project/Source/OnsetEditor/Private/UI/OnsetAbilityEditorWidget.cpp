// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/OnsetAbilityEditorWidget.h"

#include "Framework/Docking/TabManager.h"
#include "GameplayTagsManager.h"
#include "OnsetEditor.h"
#include "PackageTools.h"
#include "Blueprint/WidgetTree.h"
#include "Combat/OnsetAbilityLibrary.h"
#include "Combat/OnsetGameplayAbility.h"
#include "Components/DetailsView.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Data/OnsetAbilityTypes.h"
#include "Data/OnsetItemLibrary.h"
#include "Data/OnsetItemTypes.h"
#include "Engine/DataTable.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/Char.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "Styling/StyleDefaults.h"
#include "UI/OnsetAbilityCreationDialog.h"
#include "UI/OnsetAbilityRowButton.h"
#include "UObject/Class.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/SWindow.h"

namespace OnsetAbilityEditorWidgetConstants
{
	constexpr float ListEntryHeight = 22.0f;

	/** Extra horizontal padding per cell so text isn't flush against the column edge. */
	constexpr float NamePadding = 8.0f;
	constexpr float OtherPadding = 12.0f;
}

/** Generates a unique widget name so WidgetTree names never collide across rows. */
static FName NextWidgetName(const FString& Base)
{
	static int32 WidgetCounter = 0;
	return FName(*FString::Printf(TEXT("%s_%d"), *Base, ++WidgetCounter));
}

/** Pixel width of a string in the given font, with a fallback estimate when the measure service isn't ready. */
static float MeasureTextWidth(const FString& Text, const FSlateFontInfo& Font)
{
	const TSharedPtr<FSlateFontMeasure>& Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	if (Measure.IsValid())
	{
		const FVector2f Size = Measure->Measure(Text, Font, 1.0f);
		if (Size.X > 0.0f)
		{
			return Size.X;
		}
	}
	return static_cast<float>(Text.Len()) * 7.0f;
}

/** Maximum row-name length; keeps generated names well under FName's truncation limit. */
constexpr int32 MaxCleanNameLength = 60;

/**
 * Sanitizes a display name into a valid DataTable row name / gameplay tag segment.
 * Keeps A-Z a-z 0-9, collapses runs of separators (spaces, punctuation, '_') into a
 * single '_', and never produces a leading or trailing '_'. Falls back to "Ability".
 */
static FString CleanName(const FString& RawName)
{
	const FString Trimmed = RawName.TrimStartAndEnd();
	FString Clean;
	Clean.Reserve(FMath::Min(Trimmed.Len(), MaxCleanNameLength));

	for (const TCHAR Char : Trimmed)
	{
		if (Clean.Len() >= MaxCleanNameLength)
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
		Clean = TEXT("Ability");
	}

	return Clean;
}

/**
 * Computes the column widths:
 *  - Name: encompasses the longest name currently in the list.
 *  - Type: fixed width that accommodates the longest type display name (all enum values).
 *  - Input & Cooldown: fixed widths, equal to one another.
 */
static void ComputeColumnWidths(const UDataTable* Table, TArray<float>& OutWidths)
{
	const FSlateFontInfo RowFont = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10);
	const FSlateFontInfo HeaderFont = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 10);

	float NameW = MeasureTextWidth(NSLOCTEXT("OnsetAbilityEditor", "ColumnName", "Name").ToString(), HeaderFont);
	float TypeW = MeasureTextWidth(NSLOCTEXT("OnsetAbilityEditor", "ColumnType", "Type").ToString(), HeaderFont);
	float InputW = MeasureTextWidth(NSLOCTEXT("OnsetAbilityEditor", "ColumnInput", "Input").ToString(), HeaderFont);
	float CooldownW = MeasureTextWidth(NSLOCTEXT("OnsetAbilityEditor", "ColumnCooldown", "Cooldown").ToString(), HeaderFont);

	UEnum* AbilityTypeEnum = StaticEnum<EOnsetAbilityType>();
	if (AbilityTypeEnum)
	{
		for (int32 Index = 0; Index < AbilityTypeEnum->NumEnums(); ++Index)
		{
			const EOnsetAbilityType EnumValue = static_cast<EOnsetAbilityType>(AbilityTypeEnum->GetValueByIndex(Index));
			const FString TypeName = AbilityTypeEnum->GetDisplayValueAsText(EnumValue).ToString();
			TypeW = FMath::Max(TypeW, MeasureTextWidth(TypeName, RowFont));
		}
	}

	if (Table)
	{
		for (const TPair<FName, uint8*>& RowPair : Table->GetRowMap())
		{
			const FOnsetAbilityDefinition* Definition = reinterpret_cast<const FOnsetAbilityDefinition*>(RowPair.Value);
			if (!Definition)
			{
				continue;
			}

			const FString NameText = Definition->DisplayName.IsEmpty() ? RowPair.Key.ToString() : Definition->DisplayName.ToString();
			NameW = FMath::Max(NameW, MeasureTextWidth(NameText, RowFont));

			const FString InputText = (Definition->InputID == INDEX_NONE) ? FString(TEXT("—")) : FString::FromInt(Definition->InputID);
			InputW = FMath::Max(InputW, MeasureTextWidth(InputText, RowFont));

			const FString CooldownText = FString::Printf(TEXT("%.1fs"), Definition->CooldownSeconds);
			CooldownW = FMath::Max(CooldownW, MeasureTextWidth(CooldownText, RowFont));
		}
	}

	const float InputCooldownW = FMath::Max(InputW, CooldownW);

	OutWidths.SetNum(4);
	OutWidths[0] = NameW + OnsetAbilityEditorWidgetConstants::NamePadding;
	OutWidths[1] = TypeW + OnsetAbilityEditorWidgetConstants::OtherPadding;
	OutWidths[2] = InputCooldownW + OnsetAbilityEditorWidgetConstants::OtherPadding;
	OutWidths[3] = InputCooldownW + OnsetAbilityEditorWidgetConstants::OtherPadding;
}

/** Flat, compact button style: no chrome, subtle hover/press fill. */
static FButtonStyle MakeBaseFlatStyle()
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
static FButtonStyle MakeRowStyle(bool bSelected)
{
	FButtonStyle Style = MakeBaseFlatStyle();
	// Zero horizontal padding so the row content aligns with the column header.
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
static USizeBox* MakeCell(UWidgetTree* WidgetTree, const FString& BaseName, float Width, const FText& Text, bool bHeader, ETextJustify::Type Justification)
{
	USizeBox* CellBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), NextWidgetName(BaseName + TEXT("_Cell")));
	CellBox->SetWidthOverride(Width);

	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), NextWidgetName(BaseName + TEXT("_Label")));
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
static UButton* MakeTextButton(UWidgetTree* WidgetTree, const FText& Label, const FString& BaseName)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), NextWidgetName(BaseName + TEXT("_Button")));
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), NextWidgetName(BaseName + TEXT("_Label")));
	Text->SetText(Label);
	Text->SetJustification(ETextJustify::Center);
	Text->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10));
	Button->SetContent(Text);
	Button->SetStyle(MakeBaseFlatStyle());
	return Button;
}

UDataTable* UOnsetAbilityEditorWidget::GetAbilityTable()
{
	if (!CachedTable)
	{
		CachedTable = UOnsetAbilityLibrary::GetAbilityTable();
	}
	return CachedTable;
}

void UOnsetAbilityEditorWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildUI();
	RefreshFromTable();
}

void UOnsetAbilityEditorWidget::OpenEditor()
{
	BuildUI();
	RefreshFromTable();
}

void UOnsetAbilityEditorWidget::BuildUI()
{
	if (WidgetTree && WidgetTree->RootWidget)
	{
		return;
	}

	// Root: vertical — content (list + details) on top, buttons pinned to the bottom.
	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootLayout"));
	UHorizontalBox* ContentRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ContentRow"));

	// --- Left: column header + scrollable spreadsheet list ---
	UVerticalBox* LeftPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftPanel"));

	// Size the columns to their content up front so the header matches the rows.
	TArray<float> Widths;
	ComputeColumnWidths(GetAbilityTable(), Widths);
	ColumnWidths = Widths;

	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ColumnHeader"));
	HeaderCells.Reset();
	HeaderCells.Add(MakeCell(WidgetTree, TEXT("HeaderName"), Widths[0], NSLOCTEXT("OnsetAbilityEditor", "ColumnName", "Name"), true, ETextJustify::Left));
	HeaderCells.Add(MakeCell(WidgetTree, TEXT("HeaderType"), Widths[1], NSLOCTEXT("OnsetAbilityEditor", "ColumnType", "Type"), true, ETextJustify::Center));
	HeaderCells.Add(MakeCell(WidgetTree, TEXT("HeaderInput"), Widths[2], NSLOCTEXT("OnsetAbilityEditor", "ColumnInput", "Input"), true, ETextJustify::Center));
	HeaderCells.Add(MakeCell(WidgetTree, TEXT("HeaderCooldown"), Widths[3], NSLOCTEXT("OnsetAbilityEditor", "ColumnCooldown", "Cooldown"), true, ETextJustify::Center));
	for (USizeBox* Cell : HeaderCells)
	{
		HeaderRow->AddChild(Cell);
	}
	LeftPanel->AddChildToVerticalBox(HeaderRow);

	UScrollBox* ListScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RowList"));
	ListScroll->SetAlwaysShowScrollbar(true);
	ListScroll->SetAlwaysShowScrollbarTrack(true);
	LeftPanel->AddChildToVerticalBox(ListScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	RowListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RowListBox"));
	ListScroll->AddChild(RowListBox);

	// --- Right: property view bound to the selected row ---
	PropertyView = WidgetTree->ConstructWidget<UDetailsView>(UDetailsView::StaticClass(), TEXT("PropertyView"));

	// The list panel hugs its columns (minimum width); the details panel takes the rest.
	ContentRow->AddChildToHorizontalBox(LeftPanel)->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	ContentRow->AddChildToHorizontalBox(PropertyView)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	RootBox->AddChildToVerticalBox(ContentRow)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// --- Bottom: full-width action bar pinned under the content ---
	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonRow"));

	UButton* AddButton = MakeTextButton(WidgetTree, NSLOCTEXT("OnsetAbilityEditor", "Add", "Add"), TEXT("Add"));
	AddButton->OnClicked.AddDynamic(this, &UOnsetAbilityEditorWidget::AddDefinition);
	ButtonRow->AddChildToHorizontalBox(AddButton)->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));

	UButton* DeleteButton = MakeTextButton(WidgetTree, NSLOCTEXT("OnsetAbilityEditor", "Delete", "Delete"), TEXT("Delete"));
	DeleteButton->OnClicked.AddDynamic(this, &UOnsetAbilityEditorWidget::DeleteDefinition);
	ButtonRow->AddChildToHorizontalBox(DeleteButton)->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));

	UButton* SaveButton = MakeTextButton(WidgetTree, NSLOCTEXT("OnsetAbilityEditor", "Save", "Save"), TEXT("Save"));
	SaveButton->OnClicked.AddDynamic(this, &UOnsetAbilityEditorWidget::SaveDefinition);
	ButtonRow->AddChildToHorizontalBox(SaveButton);

	UVerticalBoxSlot* ButtonRowSlot = RootBox->AddChildToVerticalBox(ButtonRow);
	ButtonRowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	ButtonRowSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
	ButtonRowSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));

	WidgetTree->RootWidget = RootBox;
}

void UOnsetAbilityEditorWidget::RefreshFromTable()
{
	RebuildList();
}

void UOnsetAbilityEditorWidget::RebuildList()
{
	if (!RowListBox)
	{
		return;
	}

	RowListBox->ClearChildren();
	RowButtons.Reset();

	UDataTable* Table = GetAbilityTable();
	if (!Table)
	{
		return;
	}

	// Recompute column widths (the Name column tracks the longest name in the list)
	// and resize the header cells to match.
	TArray<float> Widths;
	ComputeColumnWidths(Table, Widths);
	ColumnWidths = Widths;
	for (int32 i = 0; i < HeaderCells.Num() && i < Widths.Num(); ++i)
	{
		if (HeaderCells[i])
		{
			HeaderCells[i]->SetWidthOverride(Widths[i]);
		}
	}

	UEnum* AbilityTypeEnum = StaticEnum<EOnsetAbilityType>();

	for (const TPair<FName, uint8*>& RowPair : Table->GetRowMap())
	{
		FName RowName = RowPair.Key;
		const FOnsetAbilityDefinition* Definition = reinterpret_cast<const FOnsetAbilityDefinition*>(RowPair.Value);
		if (!Definition)
		{
			continue;
		}

		UOnsetAbilityRowButton* RowButton = NewObject<UOnsetAbilityRowButton>(this);
		RowButton->RowName = RowName;
		RowButton->SetStyle(MakeRowStyle(RowName == SelectedRowName));

		// Row content: a fixed-height box holding the 4 fixed-width cells.
		USizeBox* RowHeightBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), NextWidgetName(TEXT("RowSize")));
		RowHeightBox->SetHeightOverride(OnsetAbilityEditorWidgetConstants::ListEntryHeight);

		UHorizontalBox* Cells = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), NextWidgetName(TEXT("RowCells")));

		const FText NameText = Definition->DisplayName.IsEmpty() ? FText::FromName(RowName) : Definition->DisplayName;
		const FText TypeText = AbilityTypeEnum ? AbilityTypeEnum->GetDisplayValueAsText(Definition->AbilityType) : FText::FromString(TEXT("?"));
		const FText InputText = (Definition->InputID == INDEX_NONE) ? FText::FromString(TEXT("—")) : FText::AsNumber(Definition->InputID);
		const FText CooldownText = FText::FromString(FString::Printf(TEXT("%.1fs"), Definition->CooldownSeconds));

		Cells->AddChild(MakeCell(WidgetTree, TEXT("Name"), Widths[0], NameText, false, ETextJustify::Left));
		Cells->AddChild(MakeCell(WidgetTree, TEXT("Type"), Widths[1], TypeText, false, ETextJustify::Center));
		Cells->AddChild(MakeCell(WidgetTree, TEXT("Input"), Widths[2], InputText, false, ETextJustify::Center));
		Cells->AddChild(MakeCell(WidgetTree, TEXT("Cooldown"), Widths[3], CooldownText, false, ETextJustify::Center));

		RowHeightBox->AddChild(Cells);
		RowButton->SetContent(RowHeightBox);

		RowButton->OnClicked.AddDynamic(RowButton, &UOnsetAbilityRowButton::HandleClicked);
		RowButton->OnRowSelected.AddUObject(this, &UOnsetAbilityEditorWidget::SelectRow);

		// Pin the row to the left edge and size it to its columns so the cells align
		// with the column header above.
		RowListBox->AddChildToVerticalBox(RowButton)->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Left);
		RowButtons.Add(RowName, RowButton);
	}
}

void UOnsetAbilityEditorWidget::SelectRow(FName RowName)
{
	UDataTable* Table = GetAbilityTable();
	if (!Table)
	{
		return;
	}

	const FOnsetAbilityDefinition* Row = Table->FindRow<FOnsetAbilityDefinition>(RowName, nullptr);
	if (!Row)
	{
		return;
	}

	SelectedRowName = RowName;

	if (!EditWrapper)
	{
		EditWrapper = NewObject<UOnsetAbilityEditRowWrapper>(this);
	}
	EditWrapper->Definition = *Row;

	if (PropertyView)
	{
		PropertyView->SetObject(EditWrapper);
	}

	// Update the selection highlight in place (avoids rebuilding the list while the
	// clicked button is still dispatching its click event).
	for (const TPair<FName, TObjectPtr<UOnsetAbilityRowButton>>& Pair : RowButtons)
	{
		if (Pair.Value)
		{
			Pair.Value->SetStyle(MakeRowStyle(Pair.Key == SelectedRowName));
		}
	}
}

void UOnsetAbilityEditorWidget::AddDefinition()
{
	UDataTable* Table = GetAbilityTable();
	if (!Table)
	{
		return;
	}

	// Gather the new ability's parameters in a modal form before creating the row.
	PendingCreationData = NewObject<UAbilityCreationData>(this, NAME_None, RF_Transient);
	PendingCreationData->DisplayName = FText::FromString(TEXT("NewAbility"));

	TSharedRef<SAbilityCreationDialog> Dialog = SNew(SAbilityCreationDialog, PendingCreationData);

	TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
	if (!ParentWindow.IsValid())
	{
		ParentWindow = FGlobalTabmanager::Get()->GetRootWindow();
	}

	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(NSLOCTEXT("OnsetAbilityEditor", "CreateAbility", "Create Ability"))
		.ClientSize(FVector2D(440.0f, 660.0f))
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

	UAbilityCreationData* Data = PendingCreationData;

	// Row name mirrors the display name, sanitized for use as an AbilityID tag and
	// deduped with a numeric suffix on collision (same rules as SaveDefinition).
	const FString Clean = CleanName(Data->DisplayName.ToString());
	FName NewRowName = FName(*Clean);
	int32 Suffix = 1;
	while (Table->FindRow<FOnsetAbilityDefinition>(NewRowName, nullptr))
	{
		NewRowName = FName(*FString::Printf(TEXT("%s%d"), *Clean, Suffix++));
	}

	FOnsetAbilityDefinition NewDefinition;
	NewDefinition.DisplayName = Data->DisplayName;
	NewDefinition.AbilityIcon = Data->AbilityIcon;
	NewDefinition.AbilityType = Data->AbilityType;
	NewDefinition.AttackRange = Data->AttackRange;
	NewDefinition.CastRange = Data->CastRange;
	NewDefinition.Radius = Data->Radius;
	NewDefinition.ConeHalfAngle = Data->ConeHalfAngle;
	NewDefinition.Montage = Data->Montage;
	NewDefinition.DamageTime = Data->DamageTime;
	NewDefinition.CooldownSeconds = Data->CooldownSeconds;
	NewDefinition.ThreatMultiplier = Data->ThreatMultiplier;
	NewDefinition.AbilityClass = UOnsetGameplayAbility::StaticClass();

	// Derive the cooldown tag from the row name (Cooldown.<RowName>) and register it,
	// matching the AbilityID.<RowName> registration in LoadTable.
	const FName CooldownTagName = FName(*FString::Printf(TEXT("Cooldown.%s"), *NewRowName.ToString()));
	UGameplayTagsManager::Get().AddNativeGameplayTag(CooldownTagName);
	NewDefinition.CooldownTag = FGameplayTag::RequestGameplayTag(CooldownTagName);

	// Effects from the dialog checkboxes.
	if (Data->bDamage)
	{
		FOnsetAbilityEffect Effect;
		Effect.Type = EOnsetAbilityEffectType::Damage;
		Effect.Magnitude = Data->DamageAmount;
		Effect.ScalingType = Data->DamageScaling;
		Effect.DamageTypeTag = UOnsetAbilityLibrary::GetElementDamageTag(Data->DamageElement);
		NewDefinition.Effects.Add(Effect);
	}
	if (Data->bHeal)
	{
		FOnsetAbilityEffect Effect;
		Effect.Type = EOnsetAbilityEffectType::Heal;
		Effect.Magnitude = Data->HealAmount;
		Effect.bFriendly = true;
		NewDefinition.Effects.Add(Effect);
	}
	if (Data->bDamageOverTime)
	{
		FOnsetAbilityEffect Effect;
		Effect.Type = EOnsetAbilityEffectType::Damage;
		Effect.Magnitude = Data->DoTDamageAmount;
		Effect.Duration = Data->DoTDuration;
		Effect.Period = Data->DoTPeriod;
		Effect.DamageTypeTag = UOnsetAbilityLibrary::GetElementDamageTag(Data->DoTElement);
		NewDefinition.Effects.Add(Effect);
	}
	if (Data->bHealOverTime)
	{
		FOnsetAbilityEffect Effect;
		Effect.Type = EOnsetAbilityEffectType::Heal;
		Effect.Magnitude = Data->HoTHealAmount;
		Effect.Duration = Data->HoTDuration;
		Effect.Period = Data->HoTPeriod;
		Effect.bFriendly = true;
		NewDefinition.Effects.Add(Effect);
	}
	if (Data->bSnare)
	{
		FOnsetAbilityEffect Effect;
		Effect.Type = EOnsetAbilityEffectType::Snare;
		Effect.Magnitude = Data->SnareMoveSpeedMult;
		Effect.Duration = Data->SnareDuration;
		NewDefinition.Effects.Add(Effect);
	}
	if (Data->bSlow)
	{
		FOnsetAbilityEffect Effect;
		Effect.Type = EOnsetAbilityEffectType::Slow;
		Effect.Magnitude = Data->SlowCooldownMult;
		Effect.Duration = Data->SlowDuration;
		NewDefinition.Effects.Add(Effect);
	}
	if (Data->bStun)
	{
		FOnsetAbilityEffect Effect;
		Effect.Type = EOnsetAbilityEffectType::Stun;
		Effect.Duration = Data->StunDuration;
		NewDefinition.Effects.Add(Effect);
	}
	if (Data->bFreeze)
	{
		FOnsetAbilityEffect Effect;
		Effect.Type = EOnsetAbilityEffectType::Freeze;
		Effect.Duration = Data->FreezeDuration;
		NewDefinition.Effects.Add(Effect);
	}
	if (Data->bInvulnerable)
	{
		FOnsetAbilityEffect Effect;
		Effect.Type = EOnsetAbilityEffectType::Invulnerable;
		Effect.Duration = Data->InvulnerableDuration;
		Effect.bFriendly = true;
		NewDefinition.Effects.Add(Effect);
	}

	Table->AddRow(NewRowName, NewDefinition);

	// Optionally also create a DT_Scrolls row that grants the new ability.
	if (Data->bCreateScroll)
	{
		UDataTable* ScrollTable = UOnsetItemLibrary::GetTable(EOnsetItemCategory::Scroll);
		if (ScrollTable)
		{
			FName ScrollRowName = NewRowName;
			int32 ScrollSuffix = 1;
			const FString ScrollBaseName = NewRowName.ToString();
			while (ScrollTable->FindRow<FOnsetScrollDefinition>(ScrollRowName, nullptr))
			{
				ScrollRowName = FName(*FString::Printf(TEXT("%s%d"), *ScrollBaseName, ScrollSuffix++));
			}

			FOnsetScrollDefinition ScrollRow;
			ScrollRow.DisplayName = Data->DisplayName;
			ScrollRow.Icon = Data->AbilityIcon;
			ScrollRow.GrantedAbility.DataTable = Table;
			ScrollRow.GrantedAbility.RowName = NewRowName;
			ScrollTable->AddRow(ScrollRowName, ScrollRow);

			// Persist both tables so the scroll's GrantedAbility is valid on disk
			// immediately (Add alone normally defers persistence to the Save button).
			Table->GetPackage()->MarkPackageDirty();
			ScrollTable->GetPackage()->MarkPackageDirty();
			UPackageTools::SavePackagesForObjects(TArray<UObject*>{ Table, ScrollTable });
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UOnsetAbilityEditorWidget::AddDefinition: scroll creation requested but DT_Scrolls could not be loaded."));
		}
	}

	PendingCreationData = nullptr;

	// Refresh the runtime registry so PIE sees the new ability immediately.
	UOnsetAbilityLibrary::Refresh();

	RefreshFromTable();
	SelectRow(NewRowName);
}

void UOnsetAbilityEditorWidget::DeleteDefinition()
{
	if (SelectedRowName.IsNone() || !CachedTable)
	{
		return;
	}

	// Remove any scroll rows that grant the deleted ability, so GrantedAbility
	// never dangles. Match on the handle rather than the row name so renamed
	// scroll rows are still cleaned up.
	bool bRemovedScrolls = false;
	UDataTable* ScrollTable = UOnsetItemLibrary::GetTable(EOnsetItemCategory::Scroll);
	if (ScrollTable)
	{
		TArray<FName> RowsToRemove;
		for (const TPair<FName, uint8*>& RowPair : ScrollTable->GetRowMap())
		{
			const FOnsetScrollDefinition* Scroll = reinterpret_cast<const FOnsetScrollDefinition*>(RowPair.Value);
			if (Scroll && Scroll->GrantedAbility.DataTable == CachedTable && Scroll->GrantedAbility.RowName == SelectedRowName)
			{
				RowsToRemove.Add(RowPair.Key);
			}
		}
		if (RowsToRemove.Num() > 0)
		{
			for (const FName& RowName : RowsToRemove)
			{
				ScrollTable->RemoveRow(RowName);
			}
			bRemovedScrolls = true;
			UE_LOG(LogTemp, Log, TEXT("UOnsetAbilityEditorWidget::DeleteDefinition: removed %d linked scroll row(s) for ability '%s'"),
				RowsToRemove.Num(), *SelectedRowName.ToString());
		}
	}

	CachedTable->RemoveRow(SelectedRowName);
	SelectedRowName = NAME_None;
	if (PropertyView)
	{
		PropertyView->SetObject(nullptr);
	}

	// Keep on-disk state consistent with scroll creation: when a linked scroll was
	// removed, persist both tables so no scroll references the deleted ability.
	if (bRemovedScrolls && ScrollTable)
	{
		CachedTable->GetPackage()->MarkPackageDirty();
		ScrollTable->GetPackage()->MarkPackageDirty();
		UPackageTools::SavePackagesForObjects(TArray<UObject*>{ CachedTable, ScrollTable });
	}

	RefreshFromTable();
}

bool UOnsetAbilityEditorWidget::WriteBackRow()
{
	if (!CachedTable || SelectedRowName.IsNone() || !EditWrapper)
	{
		return false;
	}

	FOnsetAbilityDefinition* Row = CachedTable->FindRow<FOnsetAbilityDefinition>(SelectedRowName, nullptr);
	if (!Row)
	{
		return false;
	}

	*Row = EditWrapper->Definition;
	return true;
}

void UOnsetAbilityEditorWidget::SaveDefinition()
{
	if (!WriteBackRow())
	{
		return;
	}

	if (CachedTable)
	{
		// Keep the row name in sync with the ability's display name, sanitized into a valid tag segment.
		const FString CleanNameString = CleanName(EditWrapper->Definition.DisplayName.ToString());
		if (!CleanNameString.IsEmpty())
		{
			FName FinalName = FName(*CleanNameString);
			int32 Suffix = 1;
			while (FinalName != SelectedRowName && CachedTable->FindRow<FOnsetAbilityDefinition>(FinalName, nullptr))
			{
				FinalName = FName(*FString::Printf(TEXT("%s%d"), *CleanNameString, Suffix++));
			}

			if (FinalName != SelectedRowName)
			{
				// UDataTable has no rename API in UE 5.8; remove + re-add with the new name.
				const FOnsetAbilityDefinition DefinitionData = EditWrapper->Definition;
				CachedTable->RemoveRow(SelectedRowName);
				CachedTable->AddRow(FinalName, DefinitionData);
				SelectedRowName = FinalName;
			}
		}

		UPackage* Package = CachedTable->GetPackage();
		Package->MarkPackageDirty();

		// Save the package so the change persists to disk (and therefore ships in the .pak).
		UPackageTools::SavePackagesForObjects(TArray<UObject*>{ CachedTable });
	}

	// Refresh the runtime registry so PIE sees the new data immediately.
	UOnsetAbilityLibrary::Refresh();

	RefreshFromTable();
}
