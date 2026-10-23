// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Creation/CharacterCreationFooterWidget.h"
#include "CommonButtonBase.h"
#include "CommonTextBlock.h"

void UCharacterCreationFooterWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (CreateButton)
	{
		CreateButton->OnClicked().AddUObject(this, &ThisClass::HandleCreateClicked);
	}

	if (BackButton)
	{
		BackButton->OnClicked().AddUObject(this, &ThisClass::HandleBackClicked);
	}

	if (BusySpinner)
	{
		BusySpinner->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UCharacterCreationFooterWidget::SetCreateButtonEnabled(bool bEnabled)
{
	if (CreateButton)
	{
		CreateButton->SetIsEnabled(bEnabled);
	}
}

void UCharacterCreationFooterWidget::SetBusy(bool bIsBusy)
{
	if (CreateButton)
	{
		CreateButton->SetIsEnabled(!bIsBusy);
	}

	if (BackButton)
	{
		BackButton->SetIsEnabled(!bIsBusy);
	}

	if (BusySpinner)
	{
		BusySpinner->SetVisibility(bIsBusy ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UCharacterCreationFooterWidget::SetFeedbackMessage(const FText& InMessage, bool bIsError)
{
	if (FeedbackText)
	{
		FeedbackText->SetText(InMessage);
		FeedbackText->SetColorAndOpacity(FSlateColor(bIsError ? ErrorFeedbackColor : NormalFeedbackColor));
	}
}

void UCharacterCreationFooterWidget::HandleCreateClicked()
{
	OnCreateClicked.Broadcast();
}

void UCharacterCreationFooterWidget::HandleBackClicked()
{
	OnBackClicked.Broadcast();
}
