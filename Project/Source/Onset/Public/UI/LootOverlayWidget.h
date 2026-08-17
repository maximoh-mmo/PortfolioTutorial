// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/OnsetItemTypes.h"
#include "LootOverlayWidget.generated.h"

class UVerticalBox;

/**
 * Popup overlay listing the items picked up from a looted corpse. Items auto-
 * inventory on loot, so this overlay is purely informational. Fully built in C++
 * (bottom-center panel) so it works with no authored asset, but is Blueprintable:
 * a WBP override can bind ItemList to a designer-owned container and the list is
 * populated the same way. Auto-hides after Lifetime seconds.
 */
UCLASS(Blueprintable)
class ONSET_API ULootOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Populates the item list and shows the overlay for Lifetime seconds. */
	void ShowLoot(const TArray<FOnsetInventoryEntry>& LootedItems);

	/** Seconds the overlay stays visible before auto-hiding. */
	UPROPERTY(EditDefaultsOnly, Category = "Loot Overlay")
	float Lifetime = 4.0f;

protected:
	virtual void NativeDestruct() override;

	/** Designer-bound container (WBP override); populated when present. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> ItemList;

private:
	/** Builds the default bottom-center panel when no designer WBP provides one. */
	void BuildWidgetTree();

	/** Collapses the overlay after Lifetime expires. */
	void HideLoot();

	FTimerHandle HideTimerHandle;
};
