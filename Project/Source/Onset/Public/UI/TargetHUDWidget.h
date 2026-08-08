// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/OnsetBaseCharacter.h"
#include "TargetHUDWidget.generated.h"

class AOnsetBaseCharacter;
class UAbilitySystemComponent;
struct FOnAttributeChangeData;

/**
 * Static target lifebar: a single top-centered HUD element that shows the
 * current target's health fill. The widget does NOT move or follow the target —
 * the designer anchors it once (e.g. top-center of WBP_HUD's canvas). It is used
 * for every enemy/boss; the WBP picks a "skin" (fill material/colors) based on
 * the exposed TargetType (Normal/Elite/Boss) when a target is acquired.
 * The screen-space reticle is gone: a ground decal on the target actor
 * (AOnsetBaseCharacter::SetTargetReticle) marks the current target instead.
 *
 * The health bar is a shader material in WBP_TargetHUD driven by a
 * BlueprintReadOnly TargetHealthPercent + OnTargetHealthPercentChanged event.
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

	/** Target category the WBP uses to pick a lifebar skin. */
	UPROPERTY(BlueprintReadOnly, Category = "Target HUD")
	ETargetType TargetType = ETargetType::Normal;

	/** Fired whenever TargetHealthPercent changes; the WBP drives its material fill. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Target HUD")
	void OnTargetHealthPercentChanged(float InPercent);

	/** Fired when a target is acquired; the WBP swaps its skin from TargetType. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Target HUD")
	void OnTargetAcquired(ETargetType InTargetType);

	/** Fired when the target is cleared; the WBP hides the lifebar. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Target HUD")
	void OnTargetCleared();

private:
	UPROPERTY()
	TObjectPtr<AOnsetBaseCharacter> TrackedTarget;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> BoundASC;
};
