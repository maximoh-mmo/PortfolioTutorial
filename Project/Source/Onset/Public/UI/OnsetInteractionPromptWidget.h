// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MHInteractionTypes.h"
#include "OnsetInteractionPromptWidget.generated.h"

class UTextBlock;

/**
 * Floating HUD prompt displaying the closest in-range interactable (e.g. "[E] Talk — Elder Vane").
 * Built in C++ so it works with no authored asset, and Blueprintable with BindWidgetOptional
 * for designer WBP overrides.
 */
UCLASS(Blueprintable)
class ONSET_API UOnsetInteractionPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Updates and displays the interaction prompt for the given definition. */
	void ShowPrompt(const FMHInteractionDefinition& Definition);

	/** Collapses the interaction prompt. */
	void HidePrompt();

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PromptText;

private:
	void BuildWidgetTree();
};
