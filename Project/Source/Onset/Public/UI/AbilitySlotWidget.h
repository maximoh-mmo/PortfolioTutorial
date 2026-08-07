// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NativeGameplayTags.h"
#include "AbilitySlotWidget.generated.h"

class UImage;
class UCommonTextBlock;
class UOnsetButtonBase;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilitySlotClicked, int32, SlotIndex);

/**
 * A single ability slot in the ability bar. Visually styled in a Widget
 * Blueprint (WBP_AbilitySlot); all logic lives here in C++. The slot is
 * generic and assignable: it displays whatever ability is granted on the ASC
 * for its InputID, or renders an empty placeholder icon when nothing is assigned.
 * Cooldown visuals are BP-driven: StartCooldown/EndCooldown raise blueprint
 * events the WBP uses to play a fill animation at PlayRate = 1/Duration.
 */
UCLASS(Blueprintable)
class ONSET_API UAbilitySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Binds the designer-placed children (UButton, UImage, etc.). */
	virtual void NativeConstruct() override;

	/** Sets the slot's key label and locked/empty state. */
	void SetSlotInfo(int32 InSlotIndex, const FText& InKeyLabel);

	/** Displays an assigned ability's icon + cooldown tag, unlocking the slot. */
	void SetAbility(TSoftObjectPtr<UTexture2D> InIcon, FGameplayTag InCooldownTag);

	/** Renders the slot's empty state (no ability assigned): shows the EmptyIcon placeholder. */
	void SetLocked(bool bLocked);

	/** Starts the cooldown fill for the given duration (seconds). */
	void StartCooldown(float InDuration);

	/** Ends the cooldown fill (no-op if no cooldown was active). */
	void EndCooldown();

	int32 GetSlotIndex() const { return SlotIndex; }

	/** The cooldown tag of the currently assigned ability (empty if locked). */
	FGameplayTag GetCooldownTag() const { return CooldownTag; }

	/** Broadcast when the slot button is clicked (indexed by slot, not input ID). */
	UPROPERTY(BlueprintAssignable, Category = "Ability Bar")
	FOnAbilitySlotClicked OnSlotClicked;

protected:
	/** Fired when the cooldown starts; BP plays a 1s fill at PlayRate = 1/Duration. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Ability Bar")
	void OnCooldownStarted(float InDuration);

	/** Fired when the cooldown ends; BP stops/rewinds the fill animation. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Ability Bar")
	void OnCooldownEnded();

	/** The designer must provide a button; click wires to OnSlotClicked. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOnsetButtonBase> SlotButton;

	/** Designer icon image; shown when an ability is assigned. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> AbilityIcon;

	/** Designer placeholder icon; shown when no ability is assigned. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> EmptyIcon;

	/** Designer key label (e.g. "1".."4"); always visible. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> KeyLabel;

private:
	UFUNCTION()
	void HandleClicked();

	/** Wires the button click. */
	void ApplyVisualState();

	/** Shows the assigned ability icon (collapses the empty placeholder). */
	void ShowAbilityIcon();

	/** Shows the empty placeholder icon (collapses the ability icon). */
	void ShowEmptyIcon();

	/** True while a cooldown fill is active (suppresses duplicate events). */
	bool bCooldownActive = false;

	int32 SlotIndex = INDEX_NONE;
	FGameplayTag CooldownTag;
};
