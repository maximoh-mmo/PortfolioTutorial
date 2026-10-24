// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "EditorUtilityWidget.h"
#include "OnsetAnimationProfile.h"
#include "OnsetAppearanceTypes.h"
#include "OnsetAuthoredAppearance.h"
#include "OnsetAppearanceStudioWidget.generated.h"

class UVerticalBox;
class UHorizontalBox;
class UTextBlock;
class USizeBox;
class UOnsetNotifyDetailsView;
class AOnsetAppearancePreviewActor;

/** Studio mode: authored appearances or animation profiles. */
UENUM()
enum class EOnsetAppearanceStudioMode : uint8
{
	Appearances,
	AnimationProfiles
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAppearanceStudioModeSelected, EOnsetAppearanceStudioMode);

/** Mode button for switching between Appearances and Animation Profiles. */
UCLASS()
class UOnsetAppearanceStudioModeButton : public UButton
{
	GENERATED_BODY()

public:
	EOnsetAppearanceStudioMode Mode = EOnsetAppearanceStudioMode::Appearances;
	FOnAppearanceStudioModeSelected OnModeSelected;

	UFUNCTION()
	void HandleClicked() { OnModeSelected.Broadcast(Mode); }
};

/** Row button in the asset library browser. */
UCLASS()
class UOnsetAppearanceAssetButton : public UButton
{
	GENERATED_BODY()

public:
	FName AssetId;
	TWeakObjectPtr<UObject> AssetObject;

	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAssetRowSelected, FName, UObject*);
	FOnAssetRowSelected OnAssetSelected;

	UFUNCTION()
	void HandleClicked() { OnAssetSelected.Broadcast(AssetId, AssetObject.Get()); }
};

/**
 * In-editor Appearance & Animation Studio.
 * Allows browsing, creating, duplicating, modifying, and previewing modular character
 * appearances and decoupled animation profiles with real-time 3D viewport updates.
 */
UCLASS()
class ONSETEDITOR_API UOnsetAppearanceStudioWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Appearance Studio")
	void OpenEditor();

	UFUNCTION(BlueprintCallable, Category = "Appearance Studio")
	void RefreshAssetList();

	UFUNCTION(BlueprintCallable, Category = "Appearance Studio")
	void CreateNewAsset();

	UFUNCTION(BlueprintCallable, Category = "Appearance Studio")
	void DuplicateSelectedAsset();

	UFUNCTION(BlueprintCallable, Category = "Appearance Studio")
	void DeleteSelectedAsset();

	UFUNCTION(BlueprintCallable, Category = "Appearance Studio")
	void SaveSelectedAsset();

	UFUNCTION(BlueprintCallable, Category = "Appearance Studio")
	void SaveAllAssets();

	UFUNCTION(BlueprintCallable, Category = "Appearance Studio")
	void SpawnOrFocusPreviewActor();

	UFUNCTION(BlueprintCallable, Category = "Appearance Studio")
	void TogglePreviewPlayback();

	UFUNCTION(BlueprintCallable, Category = "Appearance Studio")
	void PlayIdleStance();

	UFUNCTION(BlueprintCallable, Category = "Appearance Studio")
	void PlayProtocolSalute();

	UFUNCTION(BlueprintCallable, Category = "Appearance Studio")
	void PlayTalkGesture();

	UFUNCTION(BlueprintCallable, Category = "Appearance Studio")
	void ShowReferencePose();

	UFUNCTION(BlueprintCallable, Category = "Appearance Studio")
	void RotatePreview(float Degrees);

	UFUNCTION(BlueprintCallable, Category = "Appearance Studio")
	void RotatePreviewLeft();

	UFUNCTION(BlueprintCallable, Category = "Appearance Studio")
	void RotatePreviewRight();

	UFUNCTION(BlueprintCallable, Category = "Appearance Studio")
	void ToggleTurntable();

	UFUNCTION(BlueprintCallable, Category = "Appearance Studio")
	void ApplyToSelectedLevelActor();

	UFUNCTION(BlueprintCallable, Category = "Appearance Studio")
	void CleanupPreviewActor();

	static void CleanupAllPreviewActors();

	static void EnsureStarterAssetsSeeded();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BuildUI();
	void OnModeButtonClicked(EOnsetAppearanceStudioMode Mode);
	void UpdateModeButtonHighlight();
	void OnAssetSelected(FName AssetId, UObject* AssetObject);
	void HandlePropertyEdited();
	void UpdateStatusText(const FText& Text, const FLinearColor& Color = FLinearColor::White);
	void UpdatePreviewActorVisuals();
	AOnsetAppearancePreviewActor* GetOrCreatePreviewActor();

	EOnsetAppearanceStudioMode CurrentMode = EOnsetAppearanceStudioMode::Appearances;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UOnsetAppearanceStudioModeButton>> ModeButtons;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> AssetListBox;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> HeaderRow;

	UPROPERTY(Transient)
	TObjectPtr<UOnsetNotifyDetailsView> PropertyView;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PreviewInfoText;

	UPROPERTY(Transient)
	TObjectPtr<UObject> CurrentSelectedObject;

	FName CurrentSelectedId = NAME_None;

	TWeakObjectPtr<AOnsetAppearancePreviewActor> CachedPreviewActor;
};
