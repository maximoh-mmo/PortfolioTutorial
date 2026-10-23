// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/OnsetDialogueTypes.h"
#include "OnsetDialogueOverlayWidget.generated.h"

class AActor;
class UOnsetDialogueOverlayWidget;
class UTextBlock;
class UVerticalBox;

/**
 * Helper relay object to bind UButton::OnClicked (zero-arg dynamic delegate) to a specific ChoiceIndex.
 */
UCLASS()
class ONSET_API UOnsetDialogueChoiceButtonRelay : public UObject
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<UOnsetDialogueOverlayWidget> OwnerWidget;
	int32 ChoiceIndex = -1;

	UFUNCTION()
	void HandleButtonClicked();
};

/**
 * Self-contained branching dialogue overlay widget.
 * Displays speaker name, dialogue body text, and interactive response buttons filtered by
 * the owning player's quest state. Built in C++ so it works with no authored WBP asset,
 * while exposing BindWidgetOptional slots for designer WBP overrides.
 */
UCLASS(Blueprintable)
class ONSET_API UOnsetDialogueOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Opens or updates the dialogue window for TargetNPC at DialogueRowName. */
	void ShowDialogueNode(AActor* TargetNPC, FName DialogueRowName);

	/** Closes and collapses the dialogue window. */
	void CloseDialogue();

	/** Returns true if the dialogue window is currently visible. */
	bool IsDialogueOpen() const;

	/** Called by UOnsetDialogueChoiceButtonRelay when a choice button is clicked. */
	void HandleChoiceSelected(int32 ChoiceIndex);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SpeakerNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DialogueBodyText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> ChoicesBox;

private:
	void BuildWidgetTree();
	void AddChoiceButton(const FText& Label, int32 ChoiceIndex, bool bIsQuestAction);

	TWeakObjectPtr<AActor> CurrentNPCActor;
	FName CurrentDialogueRow = NAME_None;

	UPROPERTY()
	TArray<TObjectPtr<UOnsetDialogueChoiceButtonRelay>> ActiveRelays;
};
