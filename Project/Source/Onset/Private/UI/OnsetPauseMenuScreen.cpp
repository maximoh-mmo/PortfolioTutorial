// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/OnsetPauseMenuScreen.h"

#include "Engine/GameInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Player/OnsetPlayerController.h"
#include "Subsystem/OnsetUISubsystem.h"
#include "UI/OnsetButtonBase.h"

UOnsetPauseMenuScreen::UOnsetPauseMenuScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bUseMenuInputMode = true;
}

void UOnsetPauseMenuScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (ResumeButton)
	{
		ResumeButton->OnClicked().AddUObject(this, &UOnsetPauseMenuScreen::HandleResumeClicked);
	}
	if (SettingsButton)
	{
		SettingsButton->OnClicked().AddUObject(this, &UOnsetPauseMenuScreen::HandleSettingsClicked);
	}
	if (CharacterSelectButton)
	{
		CharacterSelectButton->OnClicked().AddUObject(this, &UOnsetPauseMenuScreen::HandleCharacterSelectClicked);
	}
	if (QuitDesktopButton)
	{
		QuitDesktopButton->OnClicked().AddUObject(this, &UOnsetPauseMenuScreen::HandleQuitDesktopClicked);
	}
}

void UOnsetPauseMenuScreen::NativeDestruct()
{
	if (ResumeButton)
	{
		ResumeButton->OnClicked().RemoveAll(this);
	}
	if (SettingsButton)
	{
		SettingsButton->OnClicked().RemoveAll(this);
	}
	if (CharacterSelectButton)
	{
		CharacterSelectButton->OnClicked().RemoveAll(this);
	}
	if (QuitDesktopButton)
	{
		QuitDesktopButton->OnClicked().RemoveAll(this);
	}

	Super::NativeDestruct();
}

bool UOnsetPauseMenuScreen::NativeOnHandleBackAction()
{
	HandleResumeClicked();
	return true;
}

void UOnsetPauseMenuScreen::HandleResumeClicked()
{
	if (UOnsetUISubsystem* UI = GetGameInstance()->GetSubsystem<UOnsetUISubsystem>())
	{
		UI->PopScreen(EOnsetUILayer::Menu);
	}
}

void UOnsetPauseMenuScreen::HandleSettingsClicked()
{
	if (!SettingsScreenClass)
	{
		return;
	}

	if (UOnsetUISubsystem* UI = GetGameInstance()->GetSubsystem<UOnsetUISubsystem>())
	{
		UI->PushScreen(EOnsetUILayer::Menu, SettingsScreenClass);
	}
}

void UOnsetPauseMenuScreen::HandleCharacterSelectClicked()
{
	if (UOnsetUISubsystem* UI = GetGameInstance()->GetSubsystem<UOnsetUISubsystem>())
	{
		UI->PopScreen(EOnsetUILayer::Menu);

		if (CharacterSelectScreenClass)
		{
			UI->PushScreen(EOnsetUILayer::Menu, CharacterSelectScreenClass);
		}
	}
}

void UOnsetPauseMenuScreen::HandleQuitDesktopClicked()
{
	UKismetSystemLibrary::QuitGame(GetWorld(), GetOwningPlayer(), EQuitPreference::Quit, false);
}
