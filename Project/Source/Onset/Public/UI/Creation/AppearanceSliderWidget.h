// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "AppearanceSliderWidget.generated.h"

class UCommonButtonBase;
class UCommonTextBlock;
class USlider;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMorphAdjusted, FName, MorphId, float, NewValue);

/**
 * Morph target controller displaying feature name, percentage value, and increment/decrement buttons or slider.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class ONSET_API UAppearanceSliderWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Onset|Appearance")
	FOnMorphAdjusted OnMorphAdjusted;

	UFUNCTION(BlueprintCallable, Category = "Onset|Appearance")
	void SetupMorph(FName InMorphId, const FText& InDisplayName, float InInitialValue);

	UFUNCTION(BlueprintCallable, Category = "Onset|Appearance")
	void SetValue(float InValue);

	UFUNCTION(BlueprintPure, Category = "Onset|Appearance")
	FName GetMorphId() const { return MorphId; }

	UFUNCTION(BlueprintPure, Category = "Onset|Appearance")
	float GetCurrentValue() const { return CurrentValue; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onset|Appearance")
	float StepDelta = 0.25f;

protected:
	virtual void NativeOnInitialized() override;

	UFUNCTION()
	virtual void HandleMinusClicked();

	UFUNCTION()
	virtual void HandlePlusClicked();

	UFUNCTION()
	virtual void HandleSliderValueChanged(float NewValue);

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Onset|Appearance")
	TObjectPtr<UCommonTextBlock> FeatureLabel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Onset|Appearance")
	TObjectPtr<UCommonTextBlock> ValueLabel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|Appearance")
	TObjectPtr<UCommonButtonBase> MinusButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|Appearance")
	TObjectPtr<UCommonButtonBase> PlusButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|Appearance")
	TObjectPtr<USlider> ValueSlider;

private:
	void UpdateVisuals();

	UPROPERTY(VisibleAnywhere, Category = "Onset|Appearance")
	FName MorphId = NAME_None;

	float CurrentValue = 0.0f;
	bool bIsUpdatingFromSlider = false;
};
