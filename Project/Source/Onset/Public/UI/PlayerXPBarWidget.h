// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerXPBarWidget.generated.h"

class AOnsetPlayerCharacter;
class UCommonTextBlock;

/**
 * Player XP bar + level readout (combat-formulas §12). Binds to the owning
 * pawn's replicated Level/Experience/UnspentStatPoints via its
 * OnProgressionChanged delegate and exposes the fill ratio + level text to the
 * WBP through BlueprintReadOnly properties and an event; the bar itself is a
 * material/UProgressBar in WBP_PlayerXPBar. All logic lives here in C++.
 */
UCLASS(Blueprintable)
class ONSET_API UPlayerXPBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Binds to the player pawn's progression delegate. Safe to call once. */
	void BindToPlayerCharacter(AOnsetPlayerCharacter* InPawn);

protected:
	virtual void NativeDestruct() override;

	/** Refreshes the fill ratio, level text, and stat-points readout. */
	void RefreshProgression();

	/** Delegate handler fired on XP/level changes. */
	UFUNCTION()
	void HandleProgressionChanged(int32 NewLevel, int32 NewExperience);

	/** Current XP fill ratio (0..1); the WBP drives its bar from this. */
	UPROPERTY(BlueprintReadOnly, Category = "XPBar")
	float XPPercent = 0.0f;

	/** Current level readout. */
	UPROPERTY(BlueprintReadOnly, Category = "XPBar")
	int32 CurrentLevel = 1;

	/** XP required to reach the next level. */
	UPROPERTY(BlueprintReadOnly, Category = "XPBar")
	int32 XPToNextLevel = 0;

	/** Unspent stat points waiting to be allocated. */
	UPROPERTY(BlueprintReadOnly, Category = "XPBar")
	int32 StatPointsAvailable = 0;

	/** Designer-bound text (WBP_PlayerXPBar). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> LevelText;

	/** Designer-bound text showing "Current / Required" XP. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> XPText;

	/** Fired whenever XPPercent changes; the WBP drives its bar fill. */
	UFUNCTION(BlueprintImplementableEvent, Category = "XPBar")
	void OnXPPercentChanged(float InPercent);

private:
	UPROPERTY()
	TObjectPtr<AOnsetPlayerCharacter> BoundPawn;
};