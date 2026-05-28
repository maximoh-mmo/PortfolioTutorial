// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/OnsetPlayerController.h"

#include "Combat/AbilityTargetingLibrary.h"
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
#include "Player/OnsetPlayerState.h"
#include "UI/GamepadCursorWidget.h"

DEFINE_LOG_CATEGORY(LogGamepad);

AOnsetPlayerController::AOnsetPlayerController()
{
	UE_LOG(LogTemp, Warning, TEXT("[PC Ctor] Started"));                                                        
	CursorManager = CreateDefaultSubobject<UCursorManager>(TEXT("CursorManager"));                              
	UE_LOG(LogTemp, Warning, TEXT("[PC Ctor] CursorManager: %s valid? %d"),                                     
		*GetNameSafe(CursorManager), IsValid(CursorManager));                                                   
	TargetingComponent = CreateDefaultSubobject<UTargetingComponent>(TEXT("TargetingComponent"));               
	UE_LOG(LogTemp, Warning, TEXT("[PC Ctor] TargetingComponent: %s valid? %d"),                                
		*GetNameSafe(TargetingComponent), IsValid(TargetingComponent));           
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
		EnhancedInputComponent->BindAction(IA_Ability1, ETriggerEvent::Started, this, &AOnsetPlayerController::OnAbility1);
		EnhancedInputComponent->BindAction(IA_Ability2, ETriggerEvent::Started, this, &AOnsetPlayerController::OnAbility2);
		EnhancedInputComponent->BindAction(IA_Ability3, ETriggerEvent::Started, this, &AOnsetPlayerController::OnAbility3);
		EnhancedInputComponent->BindAction(IA_Ability4, ETriggerEvent::Started, this, &AOnsetPlayerController::OnAbility4);;
		EnhancedInputComponent->BindAction(IA_PvPToggle, ETriggerEvent::Started, this, &AOnsetPlayerController::OnPvPToggleTriggered);
		
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
	if (!TargetingComponent) return;
	
	FVector2D ScreenPos;
	if (!CursorManager->GetCursorPosition(ScreenPos)) return;
	
	FHitResult HitResult;
	if (!GetHitResultAtScreenPosition(ScreenPos, ECC_Visibility, false, HitResult)) return;
	
	AActor* HitActor = HitResult.GetActor();
	if (HitActor && HitActor->ActorHasTag("Enemy"))
	{
		TargetingComponent->SetTarget(HitActor);
	}
	else if (HitActor && TargetingComponent->IsActorTargetPVPValid(HitActor, GetPawn()))
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
		else
		{
			UAIBlueprintHelperLibrary::SimpleMoveToActor(this, HitActor);
		}
		TargetingComponent->ClearTarget();
	}	
}

static void LogAbilityTargetData(int32 AbilityIndex, const FAbilityTargetData& Data)
{
	if (Data.TargetActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability %d on %s at %s (dir: %s)"),
			AbilityIndex,
			*Data.TargetActor->GetName(),
			*Data.TargetLocation.ToString(),
			*Data.TargetDirection.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability %d — no target"), AbilityIndex);
	}
}

void AOnsetPlayerController::OnAbility1(const FInputActionValue& Value)
{
	FAbilityTargetData Data = UAbilityTargetingLibrary::GetTargetData(TargetingComponent, GetPawn());
	LogAbilityTargetData(1, Data);
}

void AOnsetPlayerController::OnAbility2(const FInputActionValue& Value)
{
	FAbilityTargetData Data = UAbilityTargetingLibrary::GetTargetData(TargetingComponent, GetPawn());
	LogAbilityTargetData(2, Data);
}

void AOnsetPlayerController::OnAbility3(const FInputActionValue& Value)
{
	FAbilityTargetData Data = UAbilityTargetingLibrary::GetTargetData(TargetingComponent, GetPawn());
	LogAbilityTargetData(3, Data);
}

void AOnsetPlayerController::OnAbility4(const FInputActionValue& Value)
{
	FAbilityTargetData Data = UAbilityTargetingLibrary::GetTargetData(TargetingComponent, GetPawn());
	LogAbilityTargetData(4, Data);
}

void AOnsetPlayerController::InjectAbilityInput(int32 AbilityIndex, bool bPressed)
{
	// Route to the right action via the Enhanced Input subsystem                                               
	UEnhancedInputLocalPlayerSubsystem* Subsystem =                                                             
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());                       
	if (!Subsystem) return;                                                       
                                                                                                                     
	UInputAction* Action = nullptr;                                                                             
	switch (AbilityIndex)                                                                                       
	{                                                                                                           
	case 1: Action = IA_Ability1; break;                                                                    
	case 2: Action = IA_Ability2; break;                                                                    
	case 3: Action = IA_Ability3; break;                                                                    
	case 4: Action = IA_Ability4; break;                                                                    
	default: return;                                                                                        
	}
	Subsystem->InjectInputForAction(Action, FInputActionValue(bPressed), {}, {});
}

void AOnsetPlayerController::OnPvPToggleTriggered(const FInputActionValue& Value)                               
{                                                                                                               
	AOnsetPlayerState* OnsetPlayerState = GetPlayerState<AOnsetPlayerState>();                                                
	if (!OnsetPlayerState) return;                                                                                            
	Server_SetPvPEnabled(!OnsetPlayerState->bIsPvPEnabled);                                                                   
}                                                                                                               
                                                                                                                     
void AOnsetPlayerController::Server_SetPvPEnabled_Implementation(bool bEnabled)                                 
{                                                                                                               
	AOnsetPlayerState* OnsetPlayerState = GetPlayerState<AOnsetPlayerState>();                                                
	if(OnsetPlayerState) {
		OnsetPlayerState->bIsPvPEnabled = bEnabled;
		UE_LOG(LogTemp, Warning, TEXT("PvP Mode %hs"), bEnabled ? "enabled" : "disabled");
	}
}   