// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/OnsetAbilityCreationDialog.h"

#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "UI/OnsetAbilityEditorWidget.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SWindow.h"

void SAbilityCreationDialog::Construct(const FArguments& InArgs, UAbilityCreationData* InData)
{
	Data = InData;

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.bShowObjectLabel = false;

	TSharedRef<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetObject(Data);

	ChildSlot
	[
		SNew(SBox)
		.Padding(8.0f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					DetailsView
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			.HAlign(HAlign_Right)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("OnsetAbilityCreationDialog", "Create", "Create"))
					.OnClicked(this, &SAbilityCreationDialog::OnCreate)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("OnsetAbilityCreationDialog", "Cancel", "Cancel"))
					.OnClicked(this, &SAbilityCreationDialog::OnCancel)
				]
			]
		]
	];
}

void SAbilityCreationDialog::SetOwnerWindow(const TWeakPtr<SWindow>& InOwnerWindow)
{
	OwnerWindow = InOwnerWindow;
}

FReply SAbilityCreationDialog::OnCreate()
{
	bCreate = true;
	CloseOwnerWindow();
	return FReply::Handled();
}

FReply SAbilityCreationDialog::OnCancel()
{
	bCreate = false;
	CloseOwnerWindow();
	return FReply::Handled();
}

void SAbilityCreationDialog::CloseOwnerWindow()
{
	if (OwnerWindow.IsValid())
	{
		OwnerWindow.Pin()->RequestDestroyWindow();
	}
}