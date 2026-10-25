// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/OnsetScreenBase.h"
#include "OnsetDeathScreen.generated.h"

class UCommonTextBlock;
class UOnsetButtonBase;

/** Payload sent to client when killed. */
USTRUCT(BlueprintType)
struct FOnsetDeathReport
{
	GENERATED_BODY()

	/** Display name of the attacker or hazard that eliminated the player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	FText KillerName;

	/** Durability penalty percentage incurred on death (e.g. 15%). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	float DurabilityLossPercent = 15.0f;

	/** Minimum duration in seconds before the player can respawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	float RespawnDelay = 5.0f;

	/** Name of the respawn outpost / base. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	FText RespawnLocationName;
};

/**
 * Full-screen modal Death Screen shown upon player elimination.
 * Displays killer identity, durability penalty breakdown, countdown timer,
 * and a server-authoritative Respawn button.
 */
UCLASS(Abstract, Blueprintable)
class ONSET_API UOnsetDeathScreen : public UOnsetScreenBase
{
	GENERATED_BODY()

public:
	UOnsetDeathScreen(const FObjectInitializer& ObjectInitializer);

	/** Initializes the screen with elimination details and arms the countdown. */
	UFUNCTION(BlueprintCallable, Category = "Onset|Death")
	void SetDeathReport(const FOnsetDeathReport& InReport);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual bool NativeOnHandleBackAction() override;

	/** Label showing "Eliminated by [KillerName]". */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> KillerNameText;

	/** Label detailing the equipment durability penalty (-15%). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> PenaltyText;

	/** Countdown timer text ("Respawn available in 4s..."). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> CountdownText;

	/** Outpost / base location label ("Respawn at: Outpost Alpha"). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> RespawnLocationText;

	/** Interactive button that triggers respawn once the delay elapses. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOnsetButtonBase> RespawnButton;

	/** Blueprint hook for styling, particles, or death camera transitions. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Onset|Death")
	void BP_OnDeathReportApplied(const FOnsetDeathReport& Report);

	/** Blueprint hook when respawn timer finishes and button unlocks. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Onset|Death")
	void BP_OnRespawnReady();

private:
	UFUNCTION()
	void HandleRespawnClicked();

	FOnsetDeathReport CurrentReport;
	float RemainingSeconds = 0.0f;
	bool bRespawnReady = false;
};
