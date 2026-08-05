// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DamageNumberWidget.generated.h"

class UTextBlock;

/**
 * A single floating damage number. Owned by a pool in the HUD: ShowDamage starts
 * the float animation and the widget returns to its collapsed, inactive state
 * (it never destroys itself) when the animation completes.
 */
UCLASS()
class ONSET_API UDamageNumberWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Displays an amount at a screen-space position and starts the float animation. */
	void ShowDamage(float Amount, FVector2D ScreenPosition);

	/** True while this widget is animating/visible; false once its float has finished. */
	bool IsActive() const { return bIsAnimating; }

	/** Forcibly stops the animation and returns the widget to its inactive state. */
	void Deactivate();

	/** Random horizontal/vertical offset applied per spawn so stacked hits stay readable. */
	UPROPERTY(EditDefaultsOnly, Category = "DamageNumber")
	float JitterRadius = 12.0f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Seconds the number is visible before returning to the pool. */
	UPROPERTY(EditDefaultsOnly, Category = "DamageNumber")
	float Lifetime = 0.8f;

	/** Vertical travel distance in pixels over the lifetime. */
	UPROPERTY(EditDefaultsOnly, Category = "DamageNumber")
	float FloatDistance = 60.0f;

private:
	UPROPERTY()
	TObjectPtr<UTextBlock> NumberText;

	float Elapsed = 0.0f;
	FVector2D StartPosition;
	FLinearColor StartColor;
	bool bIsAnimating = false;
};
