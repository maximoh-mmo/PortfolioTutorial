// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OnsetPlayerDataTypes.h"
#include "Blueprint/UserWidget.h"
#include "CharacterSlot.generated.h"

class UCommonButtonBase;
class UCommonTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnsetCharacterSlotClicked, int32, SlotIndex);

/**
 * Base class for a single character-select slot.
 *
 * All behavior lives in C++: the slot index, per-slot account data, the
 * occupied/empty display state (name text, level text, delete button visibility)
 * and click routing. Blueprints only provide aesthetics (layout, fonts, colors,
 * per-class visuals) via BP_OnSlotDataChanged and may read SlotData/SlotIndex
 * for extra styling.
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

protected:
	virtual void NativeOnInitialized() override;

	/** Applies the current SlotData/SlotIndex to the bound widgets, then notifies Blueprint. */
	void Refresh();

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

	/** Text displayed on an empty slot. */
	UPROPERTY(EditDefaultsOnly, Category = "Onset|CharacterSlot")
	FText EmptySlotLabel = NSLOCTEXT("CharacterSlot", "EmptySlotLabel", "Create Character");
};
