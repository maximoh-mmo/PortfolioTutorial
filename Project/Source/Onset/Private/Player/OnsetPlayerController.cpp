// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/OnsetPlayerController.h"

#include "AbilitySystemComponent.h"
#include "EnhancedInputComponent.h"
#include "Player/CursorManager.h"
#include "Player/TargetingComponent.h"
#include "EnhancedInputSubsystems.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Player/InteractionComponent.h"
#include "Player/OnsetBaseCharacter.h"
#include "Player/OnsetCheatManager.h"
#include "Player/OnsetPlayerAIController.h"
#include "Player/OnsetPlayerState.h"
#include "UI/GamepadCursorWidget.h"

DEFINE_LOG_CATEGORY(LogGamepad);

AOnsetPlayerController::AOnsetPlayerController()
{
	CursorManager = CreateDefaultSubobject<UCursorManager>(TEXT("CursorManager"));
	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComponent"));
	if (!BasicAttackAbility)
	{
		BasicAttackAbility = LoadObject<UClass>(nullptr, (TEXT("/Game/Game/Combat/GA_BasicAttack.GA_BasicAttack_C")));
	}
	CheatClass = UOnsetCheatManager::StaticClass();
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
		if (GamepadCursorWidget)
		{
			GamepadCursorWidget->SetCursorPosition(CenterPos);
		}
	}
	
	AutoCombatController = GetWorld()->SpawnActor<AOnsetPlayerAIController>(AOnsetPlayerAIController::StaticClass());
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

void AOnsetPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	TargetingComponent = GetPawn()->FindComponentByClass<UTargetingComponent>();
}

void AOnsetPlayerController::OnUnPossess()
{
	Super::OnUnPossess();
	TargetingComponent = nullptr;
}

void AOnsetPlayerController::StartAutoAttack()
{
	// Ensure we have a valid world context before setting the timer
	if (!GetWorld()) return;
	GetWorldTimerManager().SetTimer(
		AutoAttackTimerHandle,
		this,
		&AOnsetPlayerController::OnAutoAttackTick,
		AutoAttackInterval,
		true,
		0.0f);
}

void AOnsetPlayerController::StopAutoAttack()
{
	GetWorldTimerManager().ClearTimer(AutoAttackTimerHandle);
}

void AOnsetPlayerController::OnAutoAttackTick()
{
	const AOnsetBaseCharacter* Self = GetPawn<AOnsetBaseCharacter>();
	if (!Self || !Self->AbilitySystemComponent || !Self->TargetingComponent || !Self->TargetingComponent->GetTarget() || !BasicAttackAbility)
	{
		StopAutoAttack();
		return;
	}
	Self->AbilitySystemComponent->TryActivateAbilityByClass(BasicAttackAbility);
}

void AOnsetPlayerController::OnMove(const FInputActionValue& Value)
{
	if (bAutoCombatEnabled) DisableAutoCombat();
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
	if (bAutoCombatEnabled) DisableAutoCombat();
	FVector2D ScreenPos;
	if (!CursorManager->GetCursorPosition(ScreenPos)) return;
	if (InteractionComponent) InteractionComponent->ProcessPrimaryInteraction(ScreenPos);
}	

void AOnsetPlayerController::OnAbility1(const FInputActionValue& Value)
{
	if (bAutoCombatEnabled) DisableAutoCombat();
}

void AOnsetPlayerController::OnAbility2(const FInputActionValue& Value)
{	
	if (bAutoCombatEnabled) DisableAutoCombat();
}

void AOnsetPlayerController::OnAbility3(const FInputActionValue& Value)
{
	if (bAutoCombatEnabled) DisableAutoCombat();
}

void AOnsetPlayerController::OnAbility4(const FInputActionValue& Value)
{
	if (bAutoCombatEnabled) DisableAutoCombat();
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
	}
}

void AOnsetPlayerController::EnableAutoCombat()
{
	if (!AutoCombatController || bAutoCombatEnabled) return;
	if (APawn* MyPawn = GetPawn())
	{
		AutoCombatController->Possess(MyPawn);
		UnPossess();
		bAutoCombatEnabled = true;
	}
}

void AOnsetPlayerController::DisableAutoCombat()
{
	if (!AutoCombatController || !bAutoCombatEnabled) return;
	APawn* AIPawn = AutoCombatController->GetPawn();
	AutoCombatController->UnPossess();
	if (AIPawn) Possess(AIPawn);
	bAutoCombatEnabled = false;
}

void AOnsetPlayerController::ResetIdleTimer()
{
	GetWorldTimerManager().ClearTimer(IdleAutoCombatTimerHandle);
	if (IdleAutoCombatDelay > 0.0f)
	{	
		GetWorld()->GetTimerManager().SetTimer(IdleAutoCombatTimerHandle, this,
			&AOnsetPlayerController::EnableAutoCombat, IdleAutoCombatDelay,false);
	}
}
