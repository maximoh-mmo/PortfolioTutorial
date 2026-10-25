// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/OnsetSettingsScreen.h"

#include "CommonTextBlock.h"
#include "Components/Slider.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameUserSettings.h"
#include "MHAudioSubsystem.h"
#include "Subsystem/OnsetUISubsystem.h"
#include "UI/OnsetButtonBase.h"

UOnsetSettingsScreen::UOnsetSettingsScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bUseMenuInputMode = true;
}

void UOnsetSettingsScreen::NativeConstruct()
{
	Super::NativeConstruct();

	LoadCurrentSettings();

	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->SetValue(MasterVolume);
		MasterVolumeSlider->OnValueChanged.AddDynamic(this, &UOnsetSettingsScreen::HandleMasterVolumeChanged);
	}
	if (MusicVolumeSlider)
	{
		MusicVolumeSlider->SetValue(MusicVolume);
		MusicVolumeSlider->OnValueChanged.AddDynamic(this, &UOnsetSettingsScreen::HandleMusicVolumeChanged);
	}
	if (SFXVolumeSlider)
	{
		SFXVolumeSlider->SetValue(SFXVolume);
		SFXVolumeSlider->OnValueChanged.AddDynamic(this, &UOnsetSettingsScreen::HandleSFXVolumeChanged);
	}
	if (VoiceVolumeSlider)
	{
		VoiceVolumeSlider->SetValue(VoiceVolume);
		VoiceVolumeSlider->OnValueChanged.AddDynamic(this, &UOnsetSettingsScreen::HandleVoiceVolumeChanged);
	}

	if (FullscreenToggleButton)
	{
		FullscreenToggleButton->OnClicked().AddUObject(this, &UOnsetSettingsScreen::HandleFullscreenToggled);
	}
	if (ApplyButton)
	{
		ApplyButton->OnClicked().AddUObject(this, &UOnsetSettingsScreen::HandleApplyClicked);
	}
	if (ResetDefaultsButton)
	{
		ResetDefaultsButton->OnClicked().AddUObject(this, &UOnsetSettingsScreen::HandleResetDefaultsClicked);
	}
	if (BackButton)
	{
		BackButton->OnClicked().AddUObject(this, &UOnsetSettingsScreen::HandleBackClicked);
	}
}

void UOnsetSettingsScreen::NativeDestruct()
{
	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->OnValueChanged.RemoveAll(this);
	}
	if (MusicVolumeSlider)
	{
		MusicVolumeSlider->OnValueChanged.RemoveAll(this);
	}
	if (SFXVolumeSlider)
	{
		SFXVolumeSlider->OnValueChanged.RemoveAll(this);
	}
	if (VoiceVolumeSlider)
	{
		VoiceVolumeSlider->OnValueChanged.RemoveAll(this);
	}
	if (FullscreenToggleButton)
	{
		FullscreenToggleButton->OnClicked().RemoveAll(this);
	}
	if (ApplyButton)
	{
		ApplyButton->OnClicked().RemoveAll(this);
	}
	if (ResetDefaultsButton)
	{
		ResetDefaultsButton->OnClicked().RemoveAll(this);
	}
	if (BackButton)
	{
		BackButton->OnClicked().RemoveAll(this);
	}

	Super::NativeDestruct();
}

bool UOnsetSettingsScreen::NativeOnHandleBackAction()
{
	HandleBackClicked();
	return true;
}

void UOnsetSettingsScreen::LoadCurrentSettings()
{
	if (UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		bIsFullscreen = (UserSettings->GetFullscreenMode() == EWindowMode::Fullscreen);
		if (FullscreenLabel)
		{
			FullscreenLabel->SetText(bIsFullscreen ? NSLOCTEXT("OnsetSettings", "Fullscreen", "Fullscreen") : NSLOCTEXT("OnsetSettings", "Windowed", "Windowed"));
		}
	}
}

void UOnsetSettingsScreen::HandleMasterVolumeChanged(float Value)
{
	MasterVolume = FMath::Clamp(Value, 0.0f, 1.0f);
	ApplyAudioRTPCs();
}

void UOnsetSettingsScreen::HandleMusicVolumeChanged(float Value)
{
	MusicVolume = FMath::Clamp(Value, 0.0f, 1.0f);
	ApplyAudioRTPCs();
}

void UOnsetSettingsScreen::HandleSFXVolumeChanged(float Value)
{
	SFXVolume = FMath::Clamp(Value, 0.0f, 1.0f);
	ApplyAudioRTPCs();
}

void UOnsetSettingsScreen::HandleVoiceVolumeChanged(float Value)
{
	VoiceVolume = FMath::Clamp(Value, 0.0f, 1.0f);
	ApplyAudioRTPCs();
}

void UOnsetSettingsScreen::ApplyAudioRTPCs()
{
	if (UMHAudioSubsystem* Audio = UMHAudioSubsystem::Get(this))
	{
		Audio->SetRTPC(FName("MasterVolume"), MasterVolume);
		Audio->SetRTPC(FName("MusicVolume"), MusicVolume);
		Audio->SetRTPC(FName("SFXVolume"), SFXVolume);
		Audio->SetRTPC(FName("VoiceVolume"), VoiceVolume);
	}
}

void UOnsetSettingsScreen::HandleFullscreenToggled()
{
	bIsFullscreen = !bIsFullscreen;
	if (FullscreenLabel)
	{
		FullscreenLabel->SetText(bIsFullscreen ? NSLOCTEXT("OnsetSettings", "Fullscreen", "Fullscreen") : NSLOCTEXT("OnsetSettings", "Windowed", "Windowed"));
	}
}

void UOnsetSettingsScreen::HandleApplyClicked()
{
	ApplyAudioRTPCs();

	if (UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		UserSettings->SetFullscreenMode(bIsFullscreen ? EWindowMode::Fullscreen : EWindowMode::Windowed);
		UserSettings->ApplySettings(false);
	}
}

void UOnsetSettingsScreen::HandleResetDefaultsClicked()
{
	MasterVolume = 1.0f;
	MusicVolume = 1.0f;
	SFXVolume = 1.0f;
	VoiceVolume = 1.0f;

	if (MasterVolumeSlider) MasterVolumeSlider->SetValue(1.0f);
	if (MusicVolumeSlider) MusicVolumeSlider->SetValue(1.0f);
	if (SFXVolumeSlider) SFXVolumeSlider->SetValue(1.0f);
	if (VoiceVolumeSlider) VoiceVolumeSlider->SetValue(1.0f);

	ApplyAudioRTPCs();
}

void UOnsetSettingsScreen::HandleBackClicked()
{
	if (UOnsetUISubsystem* UI = GetGameInstance()->GetSubsystem<UOnsetUISubsystem>())
	{
		UI->PopScreen(EOnsetUILayer::Menu);
	}
}
