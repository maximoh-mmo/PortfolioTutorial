// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Creation/AppearanceColorPickerWidget.h"
#include "UI/Creation/OnsetColorWheel.h"
#include "Components/Slider.h"
#include "Components/Border.h"
#include "CommonTextBlock.h"

void UAppearanceColorPickerWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (ColorWheel)
	{
		ColorWheel->OnColorChanged.AddDynamic(this, &ThisClass::HandleWheelColorChanged);
	}

	if (ValueSlider)
	{
		ValueSlider->OnValueChanged.AddDynamic(this, &ThisClass::HandleBrightnessSliderChanged);
	}
}

void UAppearanceColorPickerWidget::SetupPicker(const FLinearColor& InColor, const FText& InTitle)
{
	if (TitleLabel && !InTitle.IsEmpty())
	{
		TitleLabel->SetText(InTitle);
	}

	SetColor(InColor);
}

void UAppearanceColorPickerWidget::SetColor(const FLinearColor& InColor)
{
	bIsUpdatingInternally = true;

	CurrentColor = InColor;
	const FLinearColor HSV = InColor.LinearRGBToHSV();
	CurrentHue = HSV.R;
	CurrentSaturation = HSV.G;
	CurrentBrightness = HSV.B;

	if (ColorWheel)
	{
		const FLinearColor WheelHSV(CurrentHue, CurrentSaturation, 1.0f, 1.0f);
		ColorWheel->SetSelectedColor(WheelHSV.HSVToLinearRGB());
	}

	if (ValueSlider)
	{
		ValueSlider->SetValue(CurrentBrightness);
	}

	UpdateVisuals();

	bIsUpdatingInternally = false;
}

void UAppearanceColorPickerWidget::HandleWheelColorChanged(FLinearColor WheelColor)
{
	if (bIsUpdatingInternally) return;

	const FLinearColor HSV = WheelColor.LinearRGBToHSV();
	CurrentHue = HSV.R;
	CurrentSaturation = HSV.G;

	RecomputeFinalColor();
}

void UAppearanceColorPickerWidget::HandleBrightnessSliderChanged(float NewValue)
{
	if (bIsUpdatingInternally) return;

	CurrentBrightness = FMath::Clamp(NewValue, 0.0f, 1.0f);
	RecomputeFinalColor();
}

void UAppearanceColorPickerWidget::RecomputeFinalColor()
{
	const FLinearColor FinalHSV(CurrentHue, CurrentSaturation, CurrentBrightness, 1.0f);
	CurrentColor = FinalHSV.HSVToLinearRGB();

	UpdateVisuals();
	OnColorChanged.Broadcast(CurrentColor);
}

void UAppearanceColorPickerWidget::UpdateVisuals()
{
	if (ColorPreview)
	{
		ColorPreview->SetBrushColor(CurrentColor);
	}

	if (ValueLabel)
	{
		const int32 Percent = FMath::RoundToInt(CurrentBrightness * 100.0f);
		ValueLabel->SetText(FText::Format(NSLOCTEXT("OnsetCreation", "BrightnessPercent", "{0}%"), FText::AsNumber(Percent)));
	}
}
