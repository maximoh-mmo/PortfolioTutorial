// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/JoystickWidget.h"
#include "EnhancedInputSubsystems.h"

void UJoystickWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	if (bIsTouching)
	{
		InjectMovementInput();
	}
}

void UJoystickWidget::InjectMovementInput()
{
	if (!IA_Move) return;
	
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetOwningLocalPlayer());
	
	if (Subsystem)
	{
		Subsystem->InjectInputForAction(IA_Move, FInputActionValue(CurrentAxis), {},{});
	}
}

FReply UJoystickWidget::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	TouchCenter = InGestureEvent.GetScreenSpacePosition();
	CurrentAxis = FVector2D::ZeroVector;
	bIsTouching = true;
	return FReply::Handled();
}

FReply UJoystickWidget::NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (!bIsTouching) return FReply::Handled();
	const FVector2D CurrentPos = InGestureEvent.GetScreenSpacePosition();
	FVector2D Delta = CurrentPos - TouchCenter;
	
	const float Length = Delta.Length();
	if (Length > JoystickRadius)
	{
		Delta *= JoystickRadius / Length;
	}
	
	FVector2D Normalized = Delta / JoystickRadius;
	
	if (Normalized.Length() < DeadZone)
	{
		Normalized = FVector2D::ZeroVector;
	}
	
	CurrentAxis = Normalized;
	return FReply::Handled();
}

FReply UJoystickWidget::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	CurrentAxis = FVector2D::ZeroVector;
	bIsTouching = false;
	InjectMovementInput();
	return FReply::Handled();
}
