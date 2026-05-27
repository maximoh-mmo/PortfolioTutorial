// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CursorManager.generated.h"

UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent) )
class ONSET_API UCursorManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCursorManager();
	
	virtual void InitializeComponent() override;
	
	// --- Cursor Position ---
	
	/** 
	 * Gets the active cursor position in screen space from whichever source is currently active.
	 * Returns true if successful, false otherwise (e.g., if the player controller is not valid or no position available).
	 * @param OutPosition - The output parameter that will contain the cursor position if the function returns true.
	 * @return True if the cursor position was successfully retrieved, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Cursor")
	bool GetCursorPosition(FVector2D& OutPosition) const;

	// --- Mouse ---
	// PassThrough. Mouse position is read via APlayerController::GetMousePosition
	
	// --- Touch ---
	/**
	 * Called from touch event handlers to record the last touch position.
	 * @param ScreenPosition 
	 */
	UFUNCTION(BlueprintCallable, Category = "Cursor")
	void SetTouchPosition(FVector2D ScreenPosition);
	
	// --- Gamepad ---
	/**
	 * Accumulate a 2d Delta from R-Stick into the gamepad cursor position.
	 * Typically Called from IA_Cursor binding in the PlayerController.
	 * @param Delta
	 * @param DeltaTime 
	 */
	UFUNCTION(BlueprintCallable, Category = "Cursor")
	void AddGamepadCursorDelta(const FVector2D& Delta, float DeltaTime);
	
	/** Clamp the gamepad Cursor to the viewport bounds. */
	UFUNCTION(BlueprintCallable, Category = "Cursor")
	void ClampToViewport();
	
	/** Switch the active cursor source. When gamepad is active, GetCursorPosition
	 * returns the software cursor position instead of mouse/touch. */
	UFUNCTION(BlueprintCallable, Category = "Cursor")
	void SetGamepadCursorActive(bool bActive);
	
	/** Reset the gamepad cursor to the center of the viewport. */
	UFUNCTION(BlueprintCallable, Category = "Cursor")
	void ResetGamepadCursor();

	/** Gamepad Sensitivity, exposed and editable in BP, for tuning */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cursor", meta = (ClampMin = "0.1", UIMin = "0.1"))      
	float GamepadCursorSensitivity = 800.0f;   
private:
	/** Accumulated gamepad position using R-Stick deltas. (Normalised screen coords) */
	FVector2D GamepadCursorPosition;
	/** Last recorded touch position. */
	FVector2D LastTouchPosition;
	/** Whether the gamepad R-Stick is the active cursor source. */
	bool bUsingGamepadCursor;
	
	
	/** Cached pointer to owning PlayerController (Set in InitializeComponent). */
	mutable TWeakObjectPtr<APlayerController> CachedPlayerController;
};
