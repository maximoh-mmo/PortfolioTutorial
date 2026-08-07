// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerHealthBarWidget.generated.h"

class UAbilitySystemComponent;
class UCommonTextBlock;
struct FOnAttributeChangeData;

/**
 * Player health bar. Binds to the owning character's ASC Health/MaxHealth
 * attribute change delegates and exposes the fill ratio to the WBP via a
 * BlueprintReadOnly percent + OnHealthPercentChanged event; the bar itself is
 * a shader material in WBP_PlayerHealthBar, not a UProgressBar. The visual
 * layout is styled in the WBP; this class owns all the logic.
 */
UCLASS(Blueprintable)
class ONSET_API UPlayerHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Binds to the ASC attribute change delegates. Safe to call once. */
	void BindToASC(UAbilitySystemComponent* InASC);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Reads current Health/MaxHealth and refreshes visuals. */
	void RefreshHealth();

	/** Delegate handler for ASC attribute changes. */
	void HandleAttributeChanged(const FOnAttributeChangeData& Data);

	/** Current health fill ratio (0..1); the WBP's material reads this. */
	UPROPERTY(BlueprintReadOnly, Category = "HealthBar")
	float HealthPercent = 1.0f;

	/** Designer-bound text (WBP_PlayerHealthBar). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> HealthText;

	/** Fired whenever HealthPercent changes; the WBP drives its material fill. */
	UFUNCTION(BlueprintImplementableEvent, Category = "HealthBar")
	void OnHealthPercentChanged(float InPercent);

private:
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> BoundASC;
};
