// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OnsetCastBarWidget.generated.h"

class UProgressBar;
class UCommonTextBlock;

/**
 * Tactical cast / channel bar showing active ability wind-up or channel progress.
 */
UCLASS(Abstract, Blueprintable)
class ONSET_API UOnsetCastBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Starts a cast progress display. */
	UFUNCTION(BlueprintCallable, Category = "Onset|Combat")
	void StartCast(const FText& AbilityName, float Duration, bool bIsChannel = false);

	/** Cancels or interrupts the cast. */
	UFUNCTION(BlueprintCallable, Category = "Onset|Combat")
	void InterruptCast();

	/** Completes the cast. */
	UFUNCTION(BlueprintCallable, Category = "Onset|Combat")
	void FinishCast();

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> CastProgressBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> AbilityNameText;

private:
	bool bIsCasting = false;
	bool bIsChanneling = false;
	float TotalDuration = 0.0f;
	float ElapsedTime = 0.0f;
};
