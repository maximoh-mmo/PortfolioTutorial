// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OnsetPlayerDataTypes.h"
#include "Blueprint/UserWidget.h"
#include "CharacterSlot.generated.h"

class UCommonButtonBase;
class UCommonTextBlock;
class UCanvasPanel;
class AOnsetAppearancePreviewRig;
class AOnsetAppearancePreviewActor;
class UMaterialInterface;
class UAnimSequence;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnsetCharacterSlotClicked, int32, SlotIndex);

/**
 * Base class for a single character-select slot.
 *
 * All behavior lives in C++: the slot index, per-slot account data, the
 * occupied/empty display state (name text, level text, delete button visibility),
 * 3D character model preview via AOnsetAppearancePreviewRig, and click routing.
 * Blueprints provide layout/styling and may include an optional CanvasPanel
 * named "CharacterPreview" to display the character's 3D appearance.
 */
UCLASS(Abstract)
class ONSET_API UCharacterSlot : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Assigns this slot's index and refreshes the widget. */
	void SetSlotIndex(int32 InSlotIndex);

	/** Assigns this slot's account data and refreshes the widget. */
	void SetSlotData(const FOnsetCharacterSlotData& InSlotData);

	/** True when this slot holds a created character. */
	bool IsOccupied() const { return SlotData.bOccupied; }

	/** Text to show for the slot's name ("Create Character" when empty). */
	FText GetDisplayName() const;

	/** Level text ("Level N"); empty when the slot is unoccupied. */
	FText GetDisplayLevel() const;

	UCommonButtonBase* GetInteractionButton() const { return InteractionButton; }
	UCommonButtonBase* GetDeleteButton() const { return DeleteButton; }
	int32 GetSlotIndex() const { return SlotIndex; }

	/** Fired when the slot's main interaction area is clicked. */
	UPROPERTY(BlueprintAssignable, Category = "Onset|CharacterSlot")
	FOnsetCharacterSlotClicked OnSlotActivated;

	/** Fired when the slot's delete button is clicked. */
	UPROPERTY(BlueprintAssignable, Category = "Onset|CharacterSlot")
	FOnsetCharacterSlotClicked OnDeleteRequested;

	/** Main interaction click. Bound to InteractionButton in C++; also callable from Blueprint. */
	UFUNCTION(BlueprintCallable, Category = "Onset|CharacterSlot")
	void HandleSlotClicked();

	/** Delete click. Bound to DeleteButton in C++; also callable from Blueprint. */
	UFUNCTION(BlueprintCallable, Category = "Onset|CharacterSlot")
	void HandleDeleteClicked();

	/** Enables or suspends the 3D capture rig for performance when screens deactivate. */
	void SetPreviewActive(bool bActive);

	/** Rotates the preview character model in the slot (e.g. on mouse drag or hover). */
	UFUNCTION(BlueprintCallable, Category = "Onset|CharacterSlot")
	void AddPreviewYaw(float YawDelta);

	/** Access to the underlying preview rig and actor. */
	UFUNCTION(BlueprintPure, Category = "Onset|CharacterSlot")
	AOnsetAppearancePreviewRig* GetPreviewRig() const { return PreviewRig; }

	UFUNCTION(BlueprintPure, Category = "Onset|CharacterSlot")
	AOnsetAppearancePreviewActor* GetPreviewActor() const;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Applies the current SlotData/SlotIndex to the bound widgets, then notifies Blueprint. */
	void Refresh();

	/** Ensures the 3D capture rig is spawned and attached to CharacterPreview. */
	void EnsurePreview();

	/** Updates the 3D model appearance and animation on the preview rig. */
	void RefreshPreview();

	/** Destroys the 3D capture rig and frees render target resources. */
	void DestroyPreview();

	/** Hook for Blueprint to style the slot after the logical state has been applied. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Onset|CharacterSlot")
	void BP_OnSlotDataChanged();

	UPROPERTY(BlueprintReadOnly, Category = "Onset|CharacterSlot")
	FOnsetCharacterSlotData SlotData;

	UPROPERTY(BlueprintReadOnly, Category = "Onset|CharacterSlot")
	int32 SlotIndex = -1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> NameLabel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> LevelLabel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> InteractionButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> DeleteButton;

	/** Optional canvas panel in WBP_CharacterSlot where the 3D character image is rendered. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> CharacterPreview;

	/** Text displayed on an empty slot. */
	UPROPERTY(EditDefaultsOnly, Category = "Onset|CharacterSlot")
	FText EmptySlotLabel = NSLOCTEXT("CharacterSlot", "EmptySlotLabel", "Create Character");

	/** Custom material to render the preview render target (defaults to M_CharacterPreview). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Preview")
	TSoftObjectPtr<UMaterialInterface> PreviewMaterial;

	/** Optional idle animation sequence to loop in the character slot. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Preview")
	TSoftObjectPtr<UAnimSequence> IdleAnimation;

	/** Camera position relative to the character in the isolated studio. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Preview")
	FVector CameraOffset = FVector(240.f, 0.f, 95.f);

	/** Camera rotation relative to the character in the isolated studio. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Preview")
	FRotator CameraRotation = FRotator(0.f, 180.f, 0.f);

	/** Camera field of view angle. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Preview")
	float CameraFOV = 35.f;

	/** Resolution for the 2D render target. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Preview")
	FIntPoint RenderTargetResolution = FIntPoint(512, 640);

private:
	UPROPERTY(Transient)
	TObjectPtr<AOnsetAppearancePreviewRig> PreviewRig;
};
