// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TargetHUDWidget.generated.h"

class AOnsetBaseCharacter;
class UAbilitySystemComponent;
class UBorder;
class UCommonTextBlock;
struct FOnAttributeChangeData;

/**
 * Single HUD element for the currently targeted enemy: bracketed reticle,
 * target name, and health fill. Tracks the target's world location each tick.
 * The health bar is a shader material in WBP_TargetHUD driven by a
 * BlueprintReadOnly percent + OnTargetHealthPercentChanged event; the name is
 * a UCommonTextBlock. The widget is repositioned via its own canvas slot each
 * tick. Visual layout is styled in WBP_TargetHUD (a child of WBP_HUD's canvas).
 */
UCLASS(Blueprintable)
class ONSET_API UTargetHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Sets/clears the tracked target and binds its ASC. */
	void SetTarget(AOnsetBaseCharacter* InTarget);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	void RefreshHealth();
	void HandleTargetHealthChanged(const FOnAttributeChangeData& Data);
	void SetVisibleState(bool bVisible);

	/** Current target health fill ratio (0..1); the WBP's material reads this. */
	UPROPERTY(BlueprintReadOnly, Category = "Target HUD")
	float TargetHealthPercent = 1.0f;

	/** Designer-bound reticle border (WBP_TargetHUD). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> ReticleBorder;

	/** Designer-bound target name text (WBP_TargetHUD). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> NameText;

	/** Fired whenever TargetHealthPercent changes; the WBP drives its material fill. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Target HUD")
	void OnTargetHealthPercentChanged(float InPercent);

private:
	UPROPERTY()
	TObjectPtr<AOnsetBaseCharacter> TrackedTarget;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> BoundASC;
};
