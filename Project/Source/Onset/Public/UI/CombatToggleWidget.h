// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatToggleWidget.generated.h"

class AOnsetPlayerController;
class AOnsetPlayerState;
class UCommonButtonBase;

/**
 * Autoplay + "continue on disconnect" toggles for the in-game HUD.
 * Follows the sub-widget pattern: all logic lives in C++ (state read from the
 * owning PlayerState, replicated change broadcast via OnPlayerSettingsChanged),
 * while the visual tree is owned by the WBP (WBP_CombatToggle). The WBP drives
 * its toggle visuals from the exposed BlueprintReadOnly bools + events.
 */
UCLASS(Blueprintable)
class ONSET_API UCombatToggleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Binds to the player controller and reads the initial toggle states. Safe to call once. */
	void BindToPlayer(AOnsetPlayerController* InController);

protected:
	virtual void NativeDestruct() override;

	/** Refresh handler fired when autoplay / continue-on-disconnect replicates. */
	UFUNCTION()
	void HandlePlayerSettingsChanged();

	/** Reads the owning PlayerState and exposes the current toggle states to the WBP. */
	void RefreshToggleStates();

	/** Retries subscribing to the PlayerState delegate if it wasn't replicated yet when BindToPlayer ran. */
	void RetryBindToPlayer();

	/** Subscribes the widget to the given PlayerState's settings-change delegate. */
	void SubscribeToPlayerState(AOnsetPlayerState* PS);

	/** Autoplay toggle button click. */
	UFUNCTION()
	void OnAutoplayToggled();

	/** Continue-on-disconnect toggle button click. */
	UFUNCTION()
	void OnContinueOnDisconnectToggled();

	/** Whether autoplay (AI possession) is currently enabled. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat Toggle")
	bool bAutoplayEnabled = false;

	/** Whether the pawn keeps auto-combating after this player disconnects. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat Toggle")
	bool bContinueOnDisconnect = true;

	/** Fired whenever the autoplay state changes; the WBP flips its toggle visual. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Toggle")
	void OnAutoplayStateChanged(bool bEnabled);

	/** Fired whenever the continue-on-disconnect state changes; the WBP flips its toggle visual. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Toggle")
	void OnContinueOnDisconnectStateChanged(bool bEnabled);

	/** Designer-bound autoplay toggle button (WBP_CombatToggle). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> AutoplayToggleButton;

	/** Designer-bound continue-on-disconnect toggle button (WBP_CombatToggle). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> ContinueOnDisconnectToggleButton;

private:
	UPROPERTY()
	TObjectPtr<AOnsetPlayerController> BoundController;

	UPROPERTY()
	TObjectPtr<AOnsetPlayerState> BoundPlayerState;

	/** Re-arm handle for RetryBindToPlayer while the PlayerState hasn't replicated yet. */
	FTimerHandle RetryBindTimerHandle;
};
