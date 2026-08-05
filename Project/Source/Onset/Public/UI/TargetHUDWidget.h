// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TargetHUDWidget.generated.h"

class APlayerController;
class AOnsetBaseCharacter;
class UAbilitySystemComponent;
class UBorder;
class UCanvasPanelSlot;
class UProgressBar;
class UTextBlock;
struct FOnAttributeChangeData;

/**
 * Single HUD element for the currently targeted enemy: bracketed reticle,
 * target name, and health bar. Tracks the target's world location each tick.
 */
UCLASS()
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

private:
	UPROPERTY()
	TObjectPtr<UBorder> ReticleBorder;

	UPROPERTY()
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY()
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> ContentPanelSlot;

	UPROPERTY()
	TObjectPtr<AOnsetBaseCharacter> TrackedTarget;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> BoundASC;
};
