// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "AppearanceColorPickerWidget.generated.h"

class UOnsetColorWheel;
class USlider;
class UBorder;
class UCommonTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAppearanceColorPickerChanged, const FLinearColor&, NewColor);

/**
 * Composite color picker widget combining UOnsetColorWheel with a Value/Brightness slider
 * and live color preview swatch.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class ONSET_API UAppearanceColorPickerWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Onset|Appearance")
	FOnAppearanceColorPickerChanged OnColorChanged;

	UFUNCTION(BlueprintCallable, Category = "Onset|Appearance")
	void SetupPicker(const FLinearColor& InColor, const FText& InTitle = FText::GetEmpty());

	UFUNCTION(BlueprintCallable, Category = "Onset|Appearance")
	void SetColor(const FLinearColor& InColor);

	UFUNCTION(BlueprintPure, Category = "Onset|Appearance")
	FLinearColor GetColor() const { return CurrentColor; }

protected:
	virtual void NativeOnInitialized() override;

	UFUNCTION()
	virtual void HandleWheelColorChanged(FLinearColor WheelColor);

	UFUNCTION()
	virtual void HandleBrightnessSliderChanged(float NewValue);

	void RecomputeFinalColor();
	void UpdateVisuals();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Onset|Appearance")
	TObjectPtr<UOnsetColorWheel> ColorWheel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|Appearance")
	TObjectPtr<USlider> ValueSlider;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|Appearance")
	TObjectPtr<UBorder> ColorPreview;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|Appearance")
	TObjectPtr<UCommonTextBlock> ValueLabel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|Appearance")
	TObjectPtr<UCommonTextBlock> TitleLabel;

private:
	FLinearColor CurrentColor = FLinearColor::White;
	float CurrentBrightness = 1.0f;
	float CurrentHue = 0.0f;
	float CurrentSaturation = 0.0f;

	bool bIsUpdatingInternally = false;
};
