// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NativeGameplayTags.h"
#include "AbilityBarWidget.generated.h"

class AOnsetPlayerController;
class UAbilitySystemComponent;
class UButton;
class UCanvasPanel;
class UCanvasPanelSlot;
class UProgressBar;
class UTextBlock;

/**
 * Ability bar showing the 4 ability slots (Basic, AoE, Cone, Shadowstep) with
 * cooldown overlays. Cooldown display starts on activation (cooldown tag event)
 * then polls the ASC's active gameplay effects for real remaining time while
 * any cooldown is live.
 */
UCLASS()
class ONSET_API UAbilityBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Binds to the player controller + its pawn ASC. */
	void BindToPlayer(AOnsetPlayerController* InPlayerController, UAbilitySystemComponent* InASC);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Refresh all cooldown bars; called on activation / tag change. */
	void NotifyAbilitiesChanged();

	/** Poll tick while any cooldown is active. */
	void OnCooldownTick();

	/** Returns true if at least one slot currently has an active cooldown. */
	bool RefreshCooldowns();

	/** Callback when a cooldown tag count changes on the ASC (add/remove). */
	void HandleCooldownTagChanged(const FGameplayTag Tag, int32 NewCount);

	UFUNCTION()
	void HandleSlot0Clicked();
	UFUNCTION()
	void HandleSlot1Clicked();
	UFUNCTION()
	void HandleSlot2Clicked();
	UFUNCTION()
	void HandleSlot3Clicked();

private:
	struct FAbilitySlot
	{
		UButton* Button = nullptr;
		UProgressBar* CooldownBar = nullptr;
		UTextBlock* KeyLabel = nullptr;
		UTextBlock* CooldownText = nullptr;
		FGameplayTag CooldownTag;
		int32 InputID = -1;
	};

	void BuildSlotLayout(UCanvasPanel* RootPanel);

	/** Fills in the cooldown tags for each slot. */
	void InitCooldownTags();

	/** Array aligned with the 4 slots. */
	TArray<FAbilitySlot> Slots;

	UPROPERTY()
	TObjectPtr<AOnsetPlayerController> BoundPlayerController;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> BoundASC;

	FTimerHandle CooldownTimerHandle;

	TArray<FDelegateHandle> CooldownTagHandles;

	static const TCHAR* GetSlotKeyLabel(int32 SlotIndex);
};
