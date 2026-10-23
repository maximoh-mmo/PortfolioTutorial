// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "OnsetAppearanceTypes.h"
#include "CharacterCustomizerWidget.generated.h"

class UWidgetSwitcher;
class UCommonTabListWidgetBase;
class UScrollBox;
class UOnsetAppearanceCatalog;
class UAppearanceOptionEntryWidget;
class UAppearanceColorSwatchWidget;
class UAppearanceSliderWidget;
class UAppearanceColorPickerWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAppearanceDraftChanged, const FOnsetAppearanceRecipe&, NewRecipe);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCameraFramingRequested, bool, bFaceCloseUp);

/**
 * Categorized appearance editor driving option lists from UOnsetAppearanceCatalog.
 * Designed to be reusable in both character creation and future cosmetic salons.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class ONSET_API UCharacterCustomizerWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Onset|Appearance")
	FOnAppearanceDraftChanged OnAppearanceDraftChanged;

	UPROPERTY(BlueprintAssignable, Category = "Onset|Appearance")
	FOnCameraFramingRequested OnCameraFramingRequested;

	UFUNCTION(BlueprintCallable, Category = "Onset|Appearance")
	void InitializeCustomizer(UOnsetAppearanceCatalog* InCatalog, const FOnsetAppearanceRecipe& InInitialRecipe);

	UFUNCTION(BlueprintCallable, Category = "Onset|Appearance")
	void SetDraftRecipe(const FOnsetAppearanceRecipe& InRecipe);

	UFUNCTION(BlueprintPure, Category = "Onset|Appearance")
	const FOnsetAppearanceRecipe& GetDraftRecipe() const { return DraftRecipe; }

	UFUNCTION(BlueprintCallable, Category = "Onset|Appearance")
	void SwitchToCategory(int32 CategoryIndex);

	// --- Blueprint Subclass Configurations ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Appearance|Classes")
	TSubclassOf<UAppearanceOptionEntryWidget> OptionButtonClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Appearance|Classes")
	TSubclassOf<UAppearanceColorSwatchWidget> ColorSwatchClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Appearance|Classes")
	TSubclassOf<UAppearanceSliderWidget> SliderRowClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Appearance|Classes")
	TSubclassOf<UAppearanceColorPickerWidget> ColorPickerClass;

protected:
	virtual void NativeConstruct() override;

	void RebuildAllCategories();
	void BuildBodyCategory();
	void BuildFaceCategory();
	void BuildHairCategory();
	void BuildOutfitCategory();
	void BuildColorCategory();

	UFUNCTION()
	void HandleBodySelected(FName BodyId);

	UFUNCTION()
	void HandleHeadSelected(FName HeadId);

	void HandlePartSelected(FName SlotId, FName PartId);

	UFUNCTION()
	void HandleBodyMorphAdjusted(FName MorphId, float NewValue);

	UFUNCTION()
	void HandleFaceMorphAdjusted(FName MorphId, float NewValue);

	UFUNCTION()
	void HandleSkinColorSelected(const FLinearColor& Color);

	UFUNCTION()
	void HandleEyeColorSelected(const FLinearColor& Color);

	void HandleOutfitColorSelected(int32 Channel, const FLinearColor& Color);

	UFUNCTION()
	void HandlePartOptionSelected(FName OptionId);

	UFUNCTION()
	void HandleOutfitPrimarySelected(const FLinearColor& Color);

	UFUNCTION()
	void HandleOutfitSecondarySelected(const FLinearColor& Color);

	UFUNCTION()
	void HandleOutfitAccentSelected(const FLinearColor& Color);

	UFUNCTION()
	void HandleHairColorSelected(const FLinearColor& Color);

	void CommitDraft(const FOnsetAppearanceRecipe& Candidate);

	// --- Bound Widgets ---

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Onset|Appearance")
	TObjectPtr<UWidgetSwitcher> CategorySwitcher;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|Appearance")
	TObjectPtr<UCommonTabListWidgetBase> CategoryTabList;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|Appearance")
	TObjectPtr<UScrollBox> BodyOptionsScroll;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|Appearance")
	TObjectPtr<UScrollBox> FaceOptionsScroll;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|Appearance")
	TObjectPtr<UScrollBox> HairOptionsScroll;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|Appearance")
	TObjectPtr<UScrollBox> OutfitOptionsScroll;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|Appearance")
	TObjectPtr<UScrollBox> ColorOptionsScroll;

	// --- Color Picker Runtime References ---

	UPROPERTY(Transient)
	TObjectPtr<UAppearanceColorPickerWidget> SkinColorPicker;

	UPROPERTY(Transient)
	TObjectPtr<UAppearanceColorPickerWidget> EyeColorPicker;

	UPROPERTY(Transient)
	TObjectPtr<UAppearanceColorPickerWidget> OutfitPrimaryColorPicker;

	UPROPERTY(Transient)
	TObjectPtr<UAppearanceColorPickerWidget> OutfitSecondaryColorPicker;

	UPROPERTY(Transient)
	TObjectPtr<UAppearanceColorPickerWidget> OutfitAccentColorPicker;

	UPROPERTY(Transient)
	TObjectPtr<UAppearanceColorPickerWidget> HairColorPicker;

private:
	UPROPERTY(Transient)
	TObjectPtr<UOnsetAppearanceCatalog> Catalog;

	UPROPERTY(Transient)
	FOnsetAppearanceRecipe DraftRecipe;

	bool bIsInitialized = false;
};
