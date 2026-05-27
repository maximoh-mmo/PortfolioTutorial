// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/OnsetPlayerController.h"

#include "EnhancedInputComponent.h"
#include "Player/CursorManager.h"
#include "Player/TargetingComponent.h"
#include "EnhancedInputSubsystems.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "UI/GamepadCursorWidget.h"

DEFINE_LOG_CATEGORY(LogGamepad);

AOnsetPlayerController::AOnsetPlayerController()
{
	
	CursorManager = CreateDefaultSubobject<UCursorManager>(TEXT("CursorManager"));
	TargetingComponent = CreateDefaultSubobject<UTargetingComponent>(TEXT("TargetingComponent"));
}

void AOnsetPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (Subsystem)
	{
		Subsystem->AddMappingContext(IMC_Touch, 0);
		Subsystem->AddMappingContext(IMC_KbMouse, 0);
		Subsystem->AddMappingContext(IMC_Gamepad, 0);
	}
	bShowMouseCursor = true;
	                                                                                             
	if (GamepadCursorWidgetClass)
	{
		GamepadCursorWidget = CreateWidget<UGamepadCursorWidget>(this, GamepadCursorWidgetClass);
	}
	
	if (GamepadCursorWidget)                                                                                   
	{                                                                                                               
		GamepadCursorWidget->AddToViewport(100);                                                                    
		GamepadCursorWidget->HideCursor();
	}                      
	CursorManager->ResetGamepadCursor();
	FVector2D CenterPos;
	if (CursorManager->GetCursorPosition(CenterPos))
	{
		GamepadCursorWidget->SetCursorPosition(CenterPos);
	}
}

void AOnsetPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);
	if (EnhancedInputComponent)
	{
		EnhancedInputComponent->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AOnsetPlayerController::OnMove);
		EnhancedInputComponent->BindAction(IA_Cursor, ETriggerEvent::Triggered, this, &AOnsetPlayerController::OnCursorMove);
		EnhancedInputComponent->BindAction(IA_Primary, ETriggerEvent::Started, this, &AOnsetPlayerController::OnPrimaryInteraction);
		EnhancedInputComponent->BindAction(IA_Cursor, ETriggerEvent::Completed, this, &AOnsetPlayerController::OnCursorMoveEnded);
	}
}

void AOnsetPlayerController::OnMove(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	if (MovementVector.IsZero()) return;	
	StopMovement();
	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->AddMovementInput(ControlledPawn->GetActorForwardVector(), MovementVector.Y);
		ControlledPawn->AddMovementInput(ControlledPawn->GetActorRightVector(), MovementVector.X);
	}
}

void AOnsetPlayerController::OnCursorMove(const FInputActionValue& Value)
{
	bShowMouseCursor = false;
	FVector2D CursorDelta = Value.Get<FVector2D>();
	if (CursorDelta.IsZero()) return;
	CursorManager->AddGamepadCursorDelta(CursorDelta, GetWorld()->GetDeltaSeconds());
	CursorManager->ClampToViewport();
	FVector2D ScreenPos;
	if (CursorManager->GetCursorPosition(ScreenPos))
	{
		GamepadCursorWidget->SetCursorPosition(ScreenPos);
		GamepadCursorWidget->ShowCursor();
		GetWorld()->GetTimerManager().ClearTimer(CursorIdleTimerHandle);
	}
}

void AOnsetPlayerController::OnCursorMoveEnded(const FInputActionValue& Value)
{
	GetWorld()->GetTimerManager().SetTimer(CursorIdleTimerHandle, this, &AOnsetPlayerController::HideGamepadCursor,
		CursorIdleDelay, false);
}

// ReSharper disable once CppMemberFunctionMayBeConst modifies GamepadCursorWidget
void AOnsetPlayerController::HideGamepadCursor()
{
	if (GamepadCursorWidget)
	{
		GamepadCursorWidget->HideCursor();
		CursorManager->ResetGamepadCursor();
	}
}

void AOnsetPlayerController::OnPrimaryInteraction(const FInputActionValue& Value)
{
	FVector2D ScreenPos;
	if (!CursorManager->GetCursorPosition(ScreenPos)) return;
	
	FHitResult HitResult;
	if (!GetHitResultAtScreenPosition(ScreenPos, ECC_Visibility, false, HitResult)) return;
	
	AActor* HitActor = HitResult.GetActor();
	if (HitActor && HitActor->ActorHasTag("Enemy"))
	{
		TargetingComponent->SetTarget(HitActor);
	}
	else
	{
		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		FNavLocation NavLoc;
		if (NavSys && NavSys->ProjectPointToNavigation(HitResult.Location, NavLoc))
		{
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, NavLoc.Location);
		}
		TargetingComponent->ClearTarget();
	}	
}
