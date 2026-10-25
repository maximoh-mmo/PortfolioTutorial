// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/OnsetQuestTypes.h"
#include "OnsetQuestTrackerWidget.generated.h"

class UOnsetQuestComponent;
class UVerticalBox;
class UCommonTextBlock;

/**
 * Tactical on-screen HUD tracker for active quests and stage objectives.
 */
UCLASS(Abstract, Blueprintable)
class ONSET_API UOnsetQuestTrackerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Binds this tracker to the player's quest component. */
	UFUNCTION(BlueprintCallable, Category = "Onset|Quest")
	void BindToQuestComponent(UOnsetQuestComponent* InQuestComponent);

protected:
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> QuestListContainer;

	/** Blueprint event fired when quest objectives update. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Onset|Quest")
	void BP_OnQuestTrackerUpdated(int32 ActiveQuestCount);

private:
	void HandleQuestsChanged();
	void RefreshQuestList();

	TWeakObjectPtr<UOnsetQuestComponent> BoundQuestComp;
};
