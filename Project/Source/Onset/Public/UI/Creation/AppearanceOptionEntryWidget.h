// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "AppearanceOptionEntryWidget.generated.h"

class UCommonButtonBase;
class UCommonTextBlock;
class UImage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAppearanceOptionSelected, FName, OptionId);

/**
 * Item button card for selecting a Body gender preset, Face style, Hair style, or clothing part.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class ONSET_API UAppearanceOptionEntryWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Onset|Appearance")
	FOnAppearanceOptionSelected OnOptionSelected;

	UFUNCTION(BlueprintCallable, Category = "Onset|Appearance")
	void SetupOption(FName InOptionId, const FText& InDisplayName, bool bInIsSelected);

	UFUNCTION(BlueprintCallable, Category = "Onset|Appearance")
	void SetSelectedVisual(bool bInIsSelected);

	UFUNCTION(BlueprintPure, Category = "Onset|Appearance")
	FName GetOptionId() const { return OptionId; }

	UFUNCTION(BlueprintPure, Category = "Onset|Appearance")
	bool IsSelected() const { return bIsSelected; }

protected:
	virtual void NativeOnInitialized() override;

	UFUNCTION()
	virtual void HandleButtonClicked();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Onset|Appearance")
	TObjectPtr<UCommonButtonBase> ActionButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Onset|Appearance")
	TObjectPtr<UCommonTextBlock> TitleLabel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|Appearance")
	TObjectPtr<UWidget> SelectedGlow;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|Appearance")
	TObjectPtr<UImage> ThumbnailImage;

private:
	UPROPERTY(VisibleAnywhere, Category = "Onset|Appearance")
	FName OptionId = NAME_None;

	bool bIsSelected = false;
};
