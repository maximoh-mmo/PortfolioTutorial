// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JoystickWidget.generated.h"

class UInputAction;
/**
 * 
 */
UCLASS()
class ONSET_API UJoystickWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	 /** Called from Blueprint Construct to set up the touch zone. */
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
	virtual FReply NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

	/** Returns the current normalized axis for the Blueprint thumb visual. */
	UFUNCTION(BlueprintPure, Category="Joystick")
	FVector2D GetNormalizedAxis() const { return CurrentAxis; }
	
	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")                    
	UInputAction* IA_Move;
	
	/** Radius in pixels that maps to a full deflection of 1.0. */
	UPROPERTY(EditDefaultsOnly, Category = "Input", meta = (ClampMin = "1.0"))
	float JoystickRadius = 80.0f;
	
	/** Magnitude below which the axis is snapped to zero. */
	UPROPERTY(EditDefaultsOnly, Category = "Input", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DeadZone = 0.15f;
	
private:
	void InjectMovementInput();
	FVector2D CurrentAxis;
	FVector2D TouchCenter;
	bool bIsTouching = false;	
};
