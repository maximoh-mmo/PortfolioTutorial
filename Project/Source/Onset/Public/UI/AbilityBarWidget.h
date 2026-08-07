// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "AbilityBarWidget.generated.h"

class AOnsetPlayerController;
class UAbilitySlotWidget;
class UAbilitySystemComponent;
class UHorizontalBox;

/**
 * Ability bar showing the assignable ability slots (bound to input IDs 1-4).
 * Slot contents are driven by the ASC: each slot displays whatever ability is
 * granted for its input ID (via FindAbilitySpecFromInputID) or renders locked
 * when nothing is assigned. Cooldown fills are event-driven: when a slot's
 * cooldown tag goes active the widget starts a scaled fill animation
 * (OnCooldownStarted), which the tag's removal ends (OnCooldownEnded). The
 * designer owns the visual tree (WBP_AbilityBar); C++ never builds widgets.
 */
UCLASS(Blueprintable)
class ONSET_API UAbilityBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UAbilityBarWidget(const FObjectInitializer& ObjectInitializer);

	/** Binds to the player controller + its pawn ASC and (re)builds the slots. */
	void BindToPlayer(AOnsetPlayerController* InPlayerController, UAbilitySystemComponent* InASC);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Callback when a cooldown tag count changes on the ASC (add/remove). */
	void HandleCooldownTagChanged(const FGameplayTag Tag, int32 NewCount);

	/** Slot click routed from the slot widget; activates via input ID. */
	UFUNCTION()
	void HandleSlotClicked(int32 SlotIndex);

private:
	/** Rebuilds all slot widgets from the ASC's granted abilities (by input ID). */
	void RebuildSlots();

	/** Spawns the 4 slot widgets into the designer-provided container. */
	void BuildSlots();

	/** Unregisters all cooldown-tag event handles from the ASC. */
	void UnregisterCooldownEvents();

	/** Starts/ends the cooldown fill on the slot matching Tag. */
	void SyncCooldownState(const FGameplayTag Tag, int32 NewCount);

	/** Returns the duration of the active cooldown effect matching Tag (0 if none). */
	float GetCooldownDuration(const FGameplayTag Tag) const;

	/** Per-slot descriptor derived from the ASC. */
	struct FSlotEntry
	{
		int32 InputID = INDEX_NONE;
		TObjectPtr<UAbilitySlotWidget> Widget = nullptr;
		FGameplayTag CooldownTag;
	};

	/** Designer container (e.g. horizontal bar) that C++ fills with slot widgets. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> SlotContainer;

	/** Slot widget class to instantiate per slot (override with WBP_AbilitySlot). */
	UPROPERTY(EditDefaultsOnly, Category = "Ability Bar")
	TSubclassOf<UAbilitySlotWidget> AbilitySlotWidgetClass;

	/** The 4 assignable slots, aligned with input IDs 1-4. */
	TArray<FSlotEntry> Slots;

	UPROPERTY()
	TObjectPtr<AOnsetPlayerController> BoundPlayerController;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> BoundASC;

	TArray<FDelegateHandle> CooldownTagHandles;
};
