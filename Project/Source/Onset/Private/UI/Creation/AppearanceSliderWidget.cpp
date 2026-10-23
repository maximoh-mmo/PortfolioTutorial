// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Creation/AppearanceSliderWidget.h"
#include "CommonButtonBase.h"
#include "CommonTextBlock.h"
#include "Components/Slider.h"

void UAppearanceSliderWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (MinusButton)
	{
		MinusButton->OnClicked().AddUObject(this, &ThisClass::HandleMinusClicked);
	}

	if (PlusButton)
	{
		PlusButton->OnClicked().AddUObject(this, &ThisClass::HandlePlusClicked);
	}

	if (ValueSlider)
	{
		ValueSlider->OnValueChanged.AddDynamic(this, &ThisClass::HandleSliderValueChanged);
	}
}

void UAppearanceSliderWidget::SetupMorph(FName InMorphId, const FText& InDisplayName, float InInitialValue)
{
	MorphId = InMorphId;

	if (FeatureLabel)
	{
		FeatureLabel->SetText(InDisplayName);
	}

	SetValue(InInitialValue);
}

void UAppearanceSliderWidget::SetValue(float InValue)
{
	CurrentValue = FMath::Clamp(InValue, 0.0f, 1.0f);
	UpdateVisuals();
}

void UAppearanceSliderWidget::UpdateVisuals()
{
	if (ValueLabel)
	{
		const int32 Percentage = FMath::RoundToInt(CurrentValue * 100.0f);
		ValueLabel->SetText(FText::Format(NSLOCTEXT("OnsetCreation", "PercentFormat", "{0}%"), FText::AsNumber(Percentage)));
	}

	if (ValueSlider && !bIsUpdatingFromSlider)
	{
		ValueSlider->SetValue(CurrentValue);
	}
}

void UAppearanceSliderWidget::HandleMinusClicked()
{
	const float NewValue = FMath::Clamp(CurrentValue - StepDelta, 0.0f, 1.0f);
	SetValue(NewValue);
	OnMorphAdjusted.Broadcast(MorphId, CurrentValue);
}

void UAppearanceSliderWidget::HandlePlusClicked()
{
	const float NewValue = FMath::Clamp(CurrentValue + StepDelta, 0.0f, 1.0f);
	SetValue(NewValue);
	OnMorphAdjusted.Broadcast(MorphId, CurrentValue);
}

void UAppearanceSliderWidget::HandleSliderValueChanged(float NewValue)
{
	bIsUpdatingFromSlider = true;
	SetValue(NewValue);
	bIsUpdatingFromSlider = false;

	OnMorphAdjusted.Broadcast(MorphId, CurrentValue);
}
