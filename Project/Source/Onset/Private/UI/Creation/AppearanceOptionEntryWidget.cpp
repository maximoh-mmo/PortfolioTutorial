// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Creation/AppearanceOptionEntryWidget.h"
#include "CommonButtonBase.h"
#include "CommonTextBlock.h"
#include "Components/Image.h"

void UAppearanceOptionEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (ActionButton)
	{
		ActionButton->OnClicked().AddUObject(this, &ThisClass::HandleButtonClicked);
	}
}

void UAppearanceOptionEntryWidget::SetupOption(FName InOptionId, const FText& InDisplayName, bool bInIsSelected)
{
	OptionId = InOptionId;

	if (TitleLabel)
	{
		TitleLabel->SetText(InDisplayName);
	}

	SetSelectedVisual(bInIsSelected);
}

void UAppearanceOptionEntryWidget::SetSelectedVisual(bool bInIsSelected)
{
	bIsSelected = bInIsSelected;

	if (ActionButton)
	{
		ActionButton->SetIsSelected(bInIsSelected);
	}

	if (SelectedGlow)
	{
		SelectedGlow->SetVisibility(bInIsSelected ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UAppearanceOptionEntryWidget::HandleButtonClicked()
{
	OnOptionSelected.Broadcast(OptionId);
}
