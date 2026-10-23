// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/OnsetScreenBase.h"
#include "OnsetAppearanceTypes.h"
#include "OnsetPlayerDataTypes.h"
#include "OnsetModularCharacterCreationScreen.generated.h"

class AOnsetPlayerController;
class UCharacterClassPickerWidget;
class UCharacterNameInputWidget;
class UCharacterCreationStageWidget;
class UCharacterCustomizerWidget;
class UCharacterCreationFooterWidget;
class UOnsetAppearanceCatalog;

/**
 * Top-level orchestrator screen for modular character creation.
 * Connects the 5 modular sub-widgets, coordinates CommonUI focus,
 * validates character data, and drives RPC orchestration with AOnsetPlayerController.
 */
UCLASS(Abstract, Config = Game)
class ONSET_API UOnsetModularCharacterCreationScreen : public UOnsetScreenBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Config, Category = "Onset|Character Creation", meta = (DisplayName = "Default Character Name"))
	FString DefaultCharacterName = TEXT("Kael");

	UFUNCTION(BlueprintCallable, Category = "Onset|Character Creation")
	void SetPlayerController(AOnsetPlayerController* Controller);

	UFUNCTION(BlueprintCallable, Category = "Onset|Character Creation")
	void SetSlotIndex(int32 InSlotIndex);

	UFUNCTION(BlueprintPure, Category = "Onset|Character Creation")
	int32 GetSlotIndex() const { return SlotIndex; }

	UFUNCTION(BlueprintPure, Category = "Onset|Character Creation")
	const FOnsetAppearanceRecipe& GetDraftAppearance() const { return DraftAppearance; }

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual void NativeDestruct() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	void CyclePanelFocus(bool bForward);
	void EnsureDraft();

	// --- Sub-widget event handlers ---

	UFUNCTION()
	void HandleNameChanged(const FString& NewName, bool bIsValid);

	UFUNCTION()
	void HandleClassSelected(EOnsetCharacterClass NewClass);

	UFUNCTION()
	void HandleAppearanceDraftChanged(const FOnsetAppearanceRecipe& NewRecipe);

	UFUNCTION()
	void HandleCameraFramingRequested(bool bFaceCloseUp);

	UFUNCTION()
	void HandleCreateClicked();

	UFUNCTION()
	void HandleBackClicked();

	UFUNCTION()
	void HandleAppearanceResult(bool bSuccess, const TArray<FOnsetAppearanceIssue>& Issues);

	// --- Bound Sub-Widgets ---

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Onset|Creation")
	TObjectPtr<UCharacterClassPickerWidget> ClassPicker;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Onset|Creation")
	TObjectPtr<UCharacterNameInputWidget> NameInput;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Onset|Creation")
	TObjectPtr<UCharacterCreationStageWidget> StageWidget;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Onset|Creation")
	TObjectPtr<UCharacterCustomizerWidget> Customizer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Onset|Creation")
	TObjectPtr<UCharacterCreationFooterWidget> Footer;

	// --- State ---

	UPROPERTY(BlueprintReadOnly, Category = "Onset|Creation")
	TObjectPtr<AOnsetPlayerController> CachedPlayerController;

	UPROPERTY(BlueprintReadOnly, Category = "Onset|Creation")
	int32 SlotIndex = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Onset|Creation")
	FOnsetAppearanceRecipe DraftAppearance;

private:
	UPROPERTY(Transient)
	TObjectPtr<UOnsetAppearanceCatalog> AppearanceCatalog;

	bool bDraftReady = false;
	bool bRequestPending = false;
};
