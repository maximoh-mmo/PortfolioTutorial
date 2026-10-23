// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "OnsetColorWheel.generated.h"

class SColorWheel;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOnsetColorWheelValueChanged, FLinearColor, NewColor);

/**
 * UMG wrapper for Slate's SColorWheel, enabling full color wheel selection in Widget Blueprints.
 */
UCLASS(BlueprintType, Blueprintable)
class ONSET_API UOnsetColorWheel : public UWidget
{
	GENERATED_BODY()

public:
	UOnsetColorWheel();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Onset|Color Wheel")
	FLinearColor SelectedColor = FLinearColor::White;

	UPROPERTY(BlueprintAssignable, Category = "Onset|Color Wheel")
	FOnOnsetColorWheelValueChanged OnColorChanged;

	UFUNCTION(BlueprintCallable, Category = "Onset|Color Wheel")
	void SetSelectedColor(FLinearColor InColor);

	UFUNCTION(BlueprintPure, Category = "Onset|Color Wheel")
	FLinearColor GetSelectedColor() const { return SelectedColor; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

private:
	void HandleOnValueChanged(FLinearColor InColor);
	FLinearColor GetCurrentColor() const { return SelectedColor; }

	TSharedPtr<SColorWheel> MyColorWheel;
};
