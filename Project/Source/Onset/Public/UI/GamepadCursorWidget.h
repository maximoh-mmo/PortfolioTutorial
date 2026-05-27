// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GamepadCursorWidget.generated.h"

class SImage;
class UCanvasPanel;
class UImage;
/**
* A software crosshair overlay that appears at the gamepad R-Stick cursor position. The PlayerController tells    
* it where to be — no tick, no timers, no cursor system references.      
 */
UCLASS()
class ONSET_API UGamepadCursorWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	/** Sets the screen-space position of the cursor crosshair. Called from player controller. */
	void SetCursorPosition(const FVector2D& ScreenPosition);
	
	/** Make the cursor visible. (R-Stick moved) */
	void ShowCursor();
	
	/** Hide the cursor. (Input switched or timeout) */
	void HideCursor();
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativePreConstruct() override;
	
	/** Screen position exposed for BP to read and position the crosshair image. */
	UPROPERTY(BlueprintReadOnly, Category="Cursor")
	FVector2D CursorPosition;

private:
	UPROPERTY()
	TObjectPtr<UImage> CrosshairImage;
	bool bIsVisible;   
};
