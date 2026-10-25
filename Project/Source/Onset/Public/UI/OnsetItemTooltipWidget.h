// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/OnsetEquipmentTypes.h"
#include "Data/OnsetItemTypes.h"
#include "OnsetItemTooltipWidget.generated.h"

class UCommonTextBlock;
class UProgressBar;
class UImage;
class UWidget;

/**
 * Inspection tooltip widget displaying equipment stat quality grades, durability,
 * malfunction warnings, effective combat stats, and duplicate infusion progress.
 */
UCLASS(Abstract, Blueprintable)
class ONSET_API UOnsetItemTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Populates the tooltip for an inventory bag item. */
	UFUNCTION(BlueprintCallable, Category = "Onset|Tooltip")
	void SetItemEntry(const FOnsetInventoryEntry& Entry);

	/** Populates the tooltip for an equipped loadout item. */
	UFUNCTION(BlueprintCallable, Category = "Onset|Tooltip")
	void SetEquippedEntry(EOnsetEquipmentSlot EquipSlot, FName RowName, const FOnsetEquipmentInstanceData& InstanceData);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> ItemNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> RarityText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> CategoryText;

	/** Stat Quality Grade badge (e.g. "Grade A (Overclocked)"). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> GradeBadgeText;

	/** Visual progress bar for remaining durability (0..1). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> DurabilityBar;

	/** Numeric durability text (e.g. "85 / 100"). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> DurabilityText;

	/** Warning banner illuminated when CurrentDurability == 0 (50% stat penalty). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> MalfunctioningBanner;

	/** Primary combat stat (Weapon Damage or Armor Defense). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> PrimaryStatText;

	/** Secondary stat attribute bonuses (STR, INT, VIT, AGI, LUK). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> SecondaryStatsText;

	/** Infusion level text (e.g. "Infusion Rank 2"). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> InfusionLevelText;

	/** Infusion progress bar towards the next grade promotion. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> InfusionProgressBar;

	/** Flavor / lore description. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> DescriptionText;

	/** Blueprint styling hook. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Onset|Tooltip")
	void BP_OnTooltipDataUpdated();
};
