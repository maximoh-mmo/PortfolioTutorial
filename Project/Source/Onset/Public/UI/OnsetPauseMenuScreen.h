// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/OnsetScreenBase.h"
#include "OnsetPauseMenuScreen.generated.h"

class UOnsetButtonBase;

/**
 * In-game pause / escape menu screen.
 * Pushed onto EOnsetUILayer::Menu when the player presses Escape during gameplay.
 */
UCLASS(Abstract, Blueprintable)
class ONSET_API UOnsetPauseMenuScreen : public UOnsetScreenBase
{
	GENERATED_BODY()

public:
	UOnsetPauseMenuScreen(const FObjectInitializer& ObjectInitializer);

	/** Optional settings screen class to open when Settings is clicked. */
	UPROPERTY(EditDefaultsOnly, Category = "Onset|PauseMenu")
	TSubclassOf<UOnsetScreenBase> SettingsScreenClass;

	/** Character select screen class to open when Return to Character Select is clicked. */
	UPROPERTY(EditDefaultsOnly, Category = "Onset|PauseMenu")
	TSubclassOf<UOnsetScreenBase> CharacterSelectScreenClass;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual bool NativeOnHandleBackAction() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOnsetButtonBase> ResumeButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOnsetButtonBase> SettingsButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOnsetButtonBase> CharacterSelectButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOnsetButtonBase> QuitDesktopButton;

private:
	UFUNCTION()
	void HandleResumeClicked();

	UFUNCTION()
	void HandleSettingsClicked();

	UFUNCTION()
	void HandleCharacterSelectClicked();

	UFUNCTION()
	void HandleQuitDesktopClicked();
};
