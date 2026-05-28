// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EngineMinimal.h"
#include "InputActionValue.h"
#include "GameFramework/PlayerController.h"
#include "OnsetPlayerController.generated.h"

class UGamepadCursorWidget;
class UCursorManager;
class UInputAction;
class UInputMappingContext;
class UTargetingComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogGamepad, Log, All);
UCLASS()
class ONSET_API AOnsetPlayerController : public APlayerController
{
	
	GENERATED_BODY()

	AOnsetPlayerController();
protected:
	virtual void BeginPlay() override;

	virtual void SetupInputComponent() override;
private:
	//  --- Input Mapping Contexts ---
	
	/** Virtual joystick + tap + virtual ability buttons for touch input. */                                      
	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputMappingContext* IMC_Touch;
	/** Mouse clicks + WASD + number keys for desktop input. */                                                   
	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputMappingContext* IMC_KbMouse;
	/** L-Stick movement + R-Stick cursor + button abilities for gamepad. */                                      
	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputMappingContext* IMC_Gamepad;
	
	// --- Input Actions ---                                                                                      
                                                                                                                     
	/** Movement input (2D axis): virtual joystick, WASD, gamepad L-Stick. */                                     
	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")                    
	UInputAction* IA_Move;
	/** Gamepad R-Stick cursor emulation (2D axis). */                                                            
	UPROPERTY(EditDefaultsOnly, Category="Input|Actions")
	UInputAction* IA_Cursor;
	/** Primary interaction (digital): tap, left-click, R-Stick click, A button.                                  
	*  Context resolution branches on hit result: enemy → target, ground → move. etc. */    
	UPROPERTY(EditDefaultsOnly, Category="Input|Actions")
	UInputAction* IA_Primary;
	
	/** Ability Input Actions */
	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	UInputAction* IA_Ability1;
	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	UInputAction* IA_Ability2;
	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	UInputAction* IA_Ability3;
	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")                                                              
	UInputAction* IA_Ability4; 
		
	// --- Cursor ---                                                                                         
	
	/** Unified cursor position from mouse, touch, or gamepad R-Stick. */                 
	UPROPERTY()
	UCursorManager* CursorManager;	
	/** Called to hide software cursor */
	void HideGamepadCursor();	
	/** Timer handle for idle hide cursor*/
	FTimerHandle CursorIdleTimerHandle;	
	/** Time in seconds after R-Stick stops moving to hide the software cursor */
	UPROPERTY(EditDefaultsOnly, Category = "Cursor")                                                                
	float CursorIdleDelay = 1.5f;
	
	/** Virtual cursor for gamepad */
	UPROPERTY(EditDefaultsOnly, Category = "Cursor")                                                                
	TSubclassOf<UGamepadCursorWidget> GamepadCursorWidgetClass;                                                     
	UPROPERTY()                                                                                                     
	TObjectPtr<UGamepadCursorWidget> GamepadCursorWidget;         
	
	// --- Targeting ---
	
	/** Stores the current target with validation. Set by context resolution in OnPrimaryInteraction. */          
	UPROPERTY()
	UTargetingComponent* TargetingComponent;
	
	// --- Input Handlers ---                                                                                     
                                                                                                                     
	/** Called when IA_Move triggers. Applies 2D movement input to the controlled pawn. */                        
	void OnMove(const FInputActionValue& Value);	                                                                                                                     
	/** Called when IA_Cursor triggers. Accumulates R-Stick delta into the gamepad cursor position. */            
	void OnCursorMove(const FInputActionValue& Value);
	/** Called when R-Stick stops moving for cursor emulation, starts timer to hide software cursor  */
	void OnCursorMoveEnded(const FInputActionValue& Value);	
	/** Called when IA_Primary triggers. Raycasts at cursor position and branches:                                
	 *  Enemy → TargetingComponent->SetTarget(), Ground → MoveToLocation(). */                                    
	void OnPrimaryInteraction(const FInputActionValue& Value);
	/** Abiliity handlers for each ability */
	void OnAbility1(const FInputActionValue& Value);
	void OnAbility2(const FInputActionValue& Value);
	void OnAbility3(const FInputActionValue& Value);
	void OnAbility4(const FInputActionValue& Value);
	
	// --- Touch bridge (so BP widget does not need subsystem access ---
	UFUNCTION(BlueprintCallable, Category="Input")
	void InjectAbilityInput(int32 AbilityIndex, bool bPressed);
	
};
