// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/CursorManager.h"

#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

// Sets default values for this component's properties
UCursorManager::UCursorManager() 
	: GamepadCursorPosition(FVector2D::ZeroVector)
	, bUsingGamepadCursor(false)
	, CachedPlayerController(nullptr)
{
	// This component does not need to tick every frame, as it only updates cursor position in response to input events.
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bWantsInitializeComponent = true;
}

void UCursorManager::InitializeComponent()
{
	Super::InitializeComponent();
	CachedPlayerController = GetOwner() ? Cast<APlayerController>(GetOwner()) : nullptr;
}

bool UCursorManager::GetCursorPosition(FVector2D& OutPosition) const
{
	if (bUsingGamepadCursor)
	{
		OutPosition = GamepadCursorPosition;
		return true;
	}
	if (!LastTouchPosition.IsZero())
	{
		OutPosition = LastTouchPosition;
		return true;
	}
	APlayerController* Controller = CachedPlayerController.Get();
	if (!Controller)	{
		Controller = Cast<APlayerController>(GetOwner());
		CachedPlayerController = Controller;
	}
	
	if (Controller)	{
		return Controller->GetMousePosition(OutPosition.X, OutPosition.Y);
	}
	
	return false;	
}

void UCursorManager::SetTouchPosition(FVector2D ScreenPosition)
{
	LastTouchPosition = ScreenPosition;
	bUsingGamepadCursor = false;
}


void UCursorManager::SetGamepadCursorActive(bool bActive)
{
	bUsingGamepadCursor = bActive;
}

void UCursorManager::ResetGamepadCursor()
{
	APlayerController* Controller = CachedPlayerController.Get();
	if (!Controller)	{
		Controller = Cast<APlayerController>(GetOwner());
		CachedPlayerController = Controller;
	}

	if (!Controller || !Controller->GetLocalPlayer()) return;
	
	FVector2D ViewportSize;	
	Controller->GetLocalPlayer()->ViewportClient->GetViewportSize(ViewportSize);
	
	GamepadCursorPosition = ViewportSize * 0.5f;
}

void UCursorManager::ClampToViewport()
{
	APlayerController* Controller = CachedPlayerController.Get();                                                       
	if (!Controller)                                                                                                    
	{                                                                                                           
		Controller = Cast<APlayerController>(GetOwner());                                                               
		CachedPlayerController = Controller;                                                                            
	}                                                                                                           
                                                                                                                     
	if (!Controller || !Controller->GetLocalPlayer()) return;                                                                   
                                                                                                                     
	FVector2D ViewportSize;                                                                                     
	Controller->GetLocalPlayer()->ViewportClient->GetViewportSize(ViewportSize);                                        
	                           
	float DPIScale = Controller->GetLocalPlayer()->ViewportClient->GetDPIScale();
	ViewportSize *= DPIScale;
	
	GamepadCursorPosition.X = FMath::Clamp(GamepadCursorPosition.X, 0.0f, ViewportSize.X);                      
	GamepadCursorPosition.Y = FMath::Clamp(GamepadCursorPosition.Y, 0.0f, ViewportSize.Y);     
}

void UCursorManager::AddGamepadCursorDelta(const FVector2D& Delta, float DeltaTime)
{
	if (Delta.IsZero()) return;
	GamepadCursorPosition += Delta * GamepadCursorSensitivity * DeltaTime;
	bUsingGamepadCursor = true;	
}