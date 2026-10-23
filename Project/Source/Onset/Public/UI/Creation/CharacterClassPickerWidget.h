// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "OnsetPlayerDataTypes.h"
#include "CharacterClassPickerWidget.generated.h"

class UCommonButtonBase;
class UCommonTextBlock;
class UImage;
class UProgressBar;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCreationClassSelected, EOnsetCharacterClass, NewClass);

/**
 * Archetype selector (Tank, DPS, Ranged, Support) displaying class details,
 * playstyle description, and base stat allocations from DT_ClassInfo.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class ONSET_API UCharacterClassPickerWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Onset|ClassPicker")
	FOnCreationClassSelected OnClassSelected;

	UFUNCTION(BlueprintCallable, Category = "Onset|ClassPicker")
	void SelectClass(EOnsetCharacterClass NewClass);

	UFUNCTION(BlueprintPure, Category = "Onset|ClassPicker")
	EOnsetCharacterClass GetSelectedClass() const { return SelectedClass; }

	UFUNCTION(BlueprintPure, Category = "Onset|ClassPicker")
	UCommonButtonBase* GetSelectedClassButton() const;

	UFUNCTION(BlueprintPure, Category = "Onset|ClassPicker")
	TArray<UCommonButtonBase*> GetAllClassButtons() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|ClassPicker")
	float MaxStatReferenceValue = 25.0f;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	void UpdateDetailsDisplay();

	// --- Class Buttons ---

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Onset|ClassPicker")
	TObjectPtr<UCommonButtonBase> TankButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Onset|ClassPicker")
	TObjectPtr<UCommonButtonBase> DpsButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Onset|ClassPicker")
	TObjectPtr<UCommonButtonBase> RangedButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Onset|ClassPicker")
	TObjectPtr<UCommonButtonBase> SupportButton;

	// --- Details Display (Optional) ---

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|ClassPicker")
	TObjectPtr<UCommonTextBlock> ClassNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|ClassPicker")
	TObjectPtr<UCommonTextBlock> ClassRoleText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|ClassPicker")
	TObjectPtr<UCommonTextBlock> ClassDescriptionText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|ClassPicker")
	TObjectPtr<UImage> ClassIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|ClassPicker")
	TObjectPtr<UProgressBar> StrengthBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|ClassPicker")
	TObjectPtr<UProgressBar> AgilityBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|ClassPicker")
	TObjectPtr<UProgressBar> IntellectBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|ClassPicker")
	TObjectPtr<UProgressBar> DefenseBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|ClassPicker")
	TObjectPtr<UProgressBar> VitalityBar;

private:
	EOnsetCharacterClass SelectedClass = EOnsetCharacterClass::Tank;
};
