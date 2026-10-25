// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/OnsetScreenBase.h"
#include "OnsetSettingsScreen.generated.h"

class USlider;
class UCommonTextBlock;
class UOnsetButtonBase;

/**
 * Settings and options screen for audio volumes, graphics, and display modes.
 * Controls master, music, SFX, and voice channels (wired to MHAudio and engine settings).
 */
UCLASS(Abstract, Blueprintable)
class ONSET_API UOnsetSettingsScreen : public UOnsetScreenBase
{
	GENERATED_BODY()

public:
	UOnsetSettingsScreen(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual bool NativeOnHandleBackAction() override;

	// --- Audio Controls ---

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> MasterVolumeSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> MusicVolumeSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> SFXVolumeSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> VoiceVolumeSlider;

	// --- Graphics / Windowing ---

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOnsetButtonBase> FullscreenToggleButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> FullscreenLabel;

	// --- Actions ---

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOnsetButtonBase> ApplyButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOnsetButtonBase> ResetDefaultsButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOnsetButtonBase> BackButton;

private:
	UFUNCTION()
	void HandleMasterVolumeChanged(float Value);

	UFUNCTION()
	void HandleMusicVolumeChanged(float Value);

	UFUNCTION()
	void HandleSFXVolumeChanged(float Value);

	UFUNCTION()
	void HandleVoiceVolumeChanged(float Value);

	UFUNCTION()
	void HandleFullscreenToggled();

	UFUNCTION()
	void HandleApplyClicked();

	UFUNCTION()
	void HandleResetDefaultsClicked();

	UFUNCTION()
	void HandleBackClicked();

	void LoadCurrentSettings();
	void ApplyAudioRTPCs();

	float MasterVolume = 1.0f;
	float MusicVolume = 1.0f;
	float SFXVolume = 1.0f;
	float VoiceVolume = 1.0f;
	bool bIsFullscreen = true;
};
