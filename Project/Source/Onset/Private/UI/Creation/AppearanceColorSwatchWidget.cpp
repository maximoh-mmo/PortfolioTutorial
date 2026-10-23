// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Creation/AppearanceColorSwatchWidget.h"
#include "CommonButtonBase.h"
#include "Components/Border.h"

void UAppearanceColorSwatchWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (SwatchButton)
	{
		SwatchButton->OnClicked().AddUObject(this, &ThisClass::HandleButtonClicked);
	}
}

void UAppearanceColorSwatchWidget::SetupSwatch(const FLinearColor& InColor, bool bInIsSelected)
{
	SwatchColor = InColor;

	if (ColorFill)
	{
		ColorFill->SetBrushColor(InColor);
	}

	SetSelectedVisual(bInIsSelected);
}

void UAppearanceColorSwatchWidget::SetSelectedVisual(bool bInIsSelected)
{
	bIsSelected = bInIsSelected;

	if (SwatchButton)
	{
		SwatchButton->SetIsSelected(bInIsSelected);
	}

	if (ActiveIndicator)
	{
		ActiveIndicator->SetVisibility(bInIsSelected ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UAppearanceColorSwatchWidget::HandleButtonClicked()
{
	OnColorSelected.Broadcast(SwatchColor);
}
