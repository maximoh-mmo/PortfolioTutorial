// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnsetAbilityEditorWidget.h"

#include "OnsetEditor.h"
#include "Combat/OnsetAbilityLibrary.h"
#include "Combat/OnsetGameplayAbility.h"
#include "Blueprint/WidgetTree.h"
#include "Components/DetailsView.h"
#include "Components/HorizontalBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Data/OnsetAbilityTypes.h"
#include "Engine/DataTable.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "PackageTools.h"
#include "Components/HorizontalBoxSlot.h"
#include "Framework/Commands/UICommandInfo.h"
#include "UI/OnsetAbilityRowButton.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

namespace OnsetAbilityEditorWidgetConstants
{
	constexpr float ListEntryHeight = 24.0f;
}

/** Creates a UButton whose content is a single centered text block. Generates unique names to avoid WidgetTree name collisions. */
static UButton* MakeTextButton(UWidgetTree* WidgetTree, const FText& Label, const FString& BaseName)
{
	static int32 ButtonCounter = 0;
	++ButtonCounter;
	FName ButtonName = FName(*FString::Printf(TEXT("%s_Button_%d"), *BaseName, ButtonCounter));
	FName LabelName = FName(*FString::Printf(TEXT("%s_Label_%d"), *BaseName, ButtonCounter));
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), LabelName);
	Text->SetText(Label);
	Text->SetJustification(ETextJustify::Center);
	Button->SetContent(Text);
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

	// Root: horizontal split — list (left) + property view (right).
	UHorizontalBox* RootBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RootLayout"));
	
	// --- Left: scrollable list + add/delete/save buttons ---
	UVerticalBox* LeftPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftPanel"));
	
	RootBox->AddChild(LeftPanel);

	UScrollBox* ListScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RowList"));
	LeftPanel->AddChild(ListScroll);

	RowListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RowListBox"));
	ListScroll->AddChild(RowListBox);

	// --- Right: property view bound to the selected row ---
	PropertyView = WidgetTree->ConstructWidget<UDetailsView>(UDetailsView::StaticClass(), TEXT("PropertyView"));
	RootBox->AddChild(PropertyView);

	// --- Bottom buttons ---
	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonRow"));
	LeftPanel->AddChild(ButtonRow);
	
	UButton* AddButton = MakeTextButton(WidgetTree, NSLOCTEXT("OnsetAbilityEditor", "Add", "Add"), TEXT("Add"));
	AddButton->OnClicked.AddDynamic(this, &UOnsetAbilityEditorWidget::AddDefinition);
	ButtonRow->AddChild(AddButton);

	UButton* DeleteButton = MakeTextButton(WidgetTree, NSLOCTEXT("OnsetAbilityEditor", "Delete", "Delete"), TEXT("Delete"));
	DeleteButton->OnClicked.AddDynamic(this, &UOnsetAbilityEditorWidget::DeleteDefinition);
	ButtonRow->AddChild(DeleteButton);

	UButton* SaveButton = MakeTextButton(WidgetTree, NSLOCTEXT("OnsetAbilityEditor", "Save", "Save"), TEXT("Save"));
	SaveButton->OnClicked.AddDynamic(this, &UOnsetAbilityEditorWidget::SaveDefinition);
	ButtonRow->AddChild(SaveButton);

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

	UDataTable* Table = GetAbilityTable();
	if (!Table)
	{
		return;
	}

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

		UTextBlock* Label = NewObject<UTextBlock>(this);
		Label->SetText(Definition->DisplayName.IsEmpty() ? FText::FromName(RowName) : Definition->DisplayName);
		Label->SetJustification(ETextJustify::Left);
		RowButton->SetContent(Label);

		RowButton->OnClicked.AddDynamic(RowButton, &UOnsetAbilityRowButton::HandleClicked);
		RowButton->OnRowSelected.AddUObject(this, &UOnsetAbilityEditorWidget::SelectRow);

		RowListBox->AddChildToVerticalBox(RowButton);
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
}

void UOnsetAbilityEditorWidget::AddDefinition()
{
	UDataTable* Table = GetAbilityTable();
	if (!Table)
	{
		return;
	}

	// Find the next available numeric row name.
	FName NewRowName = TEXT("NewAbility");
	int32 Index = 1;
	while (Table->FindRow<FOnsetAbilityDefinition>(NewRowName, nullptr))
	{
		NewRowName = FName(*FString::Printf(TEXT("NewAbility%d"), Index));
		++Index;
	}

	FOnsetAbilityDefinition NewDefinition;
	NewDefinition.DisplayName = FText::FromName(NewRowName);
	NewDefinition.AbilityClass = UOnsetGameplayAbility::StaticClass();
	Table->AddRow(NewRowName, NewDefinition);

	RefreshFromTable();
	SelectRow(NewRowName);
}

void UOnsetAbilityEditorWidget::DeleteDefinition()
{
	if (!SelectedRowName.IsNone() && CachedTable)
	{
		CachedTable->RemoveRow(SelectedRowName);
		SelectedRowName = NAME_None;
		if (PropertyView)
		{
			PropertyView->SetObject(nullptr);
		}
		RefreshFromTable();
	}
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
		UPackage* Package = CachedTable->GetPackage();
		Package->MarkPackageDirty();

		// Save the package so the change persists to disk (and therefore ships in the .pak).
		UPackageTools::SavePackagesForObjects(TArray<UObject*>{ CachedTable });
	}

	// Refresh the runtime registry so PIE sees the new data immediately.
	UOnsetAbilityLibrary::Refresh();

	RefreshFromTable();
}
