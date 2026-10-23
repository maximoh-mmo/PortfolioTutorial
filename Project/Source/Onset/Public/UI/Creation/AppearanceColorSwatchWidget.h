// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "AppearanceColorSwatchWidget.generated.h"

class UCommonButtonBase;
class UBorder;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnColorSwatchSelected, const FLinearColor&, Color);

/**
 * Selectable color chip with highlight ring for skin tones, eye colors, and outfit color channels.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class ONSET_API UAppearanceColorSwatchWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Onset|Appearance")
	FOnColorSwatchSelected OnColorSelected;

	UFUNCTION(BlueprintCallable, Category = "Onset|Appearance")
	void SetupSwatch(const FLinearColor& InColor, bool bInIsSelected);

	UFUNCTION(BlueprintCallable, Category = "Onset|Appearance")
	void SetSelectedVisual(bool bInIsSelected);

	UFUNCTION(BlueprintPure, Category = "Onset|Appearance")
	FLinearColor GetColor() const { return SwatchColor; }

	UFUNCTION(BlueprintPure, Category = "Onset|Appearance")
	bool IsSelected() const { return bIsSelected; }

protected:
	virtual void NativeOnInitialized() override;

	UFUNCTION()
	virtual void HandleButtonClicked();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Onset|Appearance")
	TObjectPtr<UCommonButtonBase> SwatchButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Onset|Appearance")
	TObjectPtr<UBorder> ColorFill;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|Appearance")
	TObjectPtr<UBorder> ActiveIndicator;

private:
	UPROPERTY(VisibleAnywhere, Category = "Onset|Appearance")
	FLinearColor SwatchColor = FLinearColor::White;

	bool bIsSelected = false;
};
