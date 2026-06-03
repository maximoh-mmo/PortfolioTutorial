// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/OnsetPlayerController.h"

#include "AbilitySystemComponent.h"
#include "Combat/AbilityTargetingLibrary.h"
#include "EnhancedInputComponent.h"
#include "Player/CursorManager.h"
#include "Player/TargetingComponent.h"
#include "EnhancedInputSubsystems.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Player/OnsetBaseCharacter.h"
#include "Player/OnsetPlayerState.h"
#include "UI/GamepadCursorWidget.h"

DEFINE_LOG_CATEGORY(LogGamepad);

AOnsetPlayerController::AOnsetPlayerController()
{
	CursorManager = CreateDefaultSubobject<UCursorManager>(TEXT("CursorManager"));
	if (!BasicAttackAbility)
	{
		BasicAttackAbility = LoadObject<UClass>(nullptr, (TEXT("/Game/Game/Combat/GA_BasicAttack.GA_BasicAttack_C")));
		if (!BasicAttackAbility)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to find Basic Attack Ability class"));
		}
	}
}

void AOnsetPlayerController::StartAutoAttack()
{
	UE_LOG(LogTemp, Warning, TEXT("Starting auto-attack"));
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

void AOnsetPlayerController::OnAutoAttackTick()
{
	UE_LOG(LogTemp, Log, TEXT("Auto-attack tick"));
	const AOnsetBaseCharacter* Self = GetPawn<AOnsetBaseCharacter>();
	if (!Self || !Self->AbilitySystemComponent || !Self->TargetingComponent || !Self->TargetingComponent->GetTarget() || !BasicAttackAbility)
	{
		UE_LOG(LogTemp, Log, TEXT("Self=%s ASC=%s TC=%s Target=%s BAA=%s"),                                                   
			Self ? TEXT("ok") : TEXT("null"),                                                                         
			Self && Self->AbilitySystemComponent ? TEXT("ok") : TEXT("null"),                                                    
			Self && Self->TargetingComponent ? TEXT("ok") : TEXT("null"),                                                        
			Self && Self->TargetingComponent && Self->TargetingComponent->GetTarget() ? *Self->TargetingComponent->GetTarget()->GetName() : TEXT("null"),                                                                         
		 BasicAttackAbility ? TEXT("ok") : TEXT("null"));  
		StopAutoAttack();
		return;
	}
	bool bActivated = Self->AbilitySystemComponent->TryActivateAbilityByClass(BasicAttackAbility);
	UE_LOG(LogTemp, Log, TEXT("TryActivateAbilityByClass returned %s"), bActivated ? TEXT("true") : TEXT("false"));
	for (const FGameplayAbilitySpec& Spec : Self->AbilitySystemComponent->GetActivatableAbilities())
	{
		UE_LOG(LogTemp, Log, TEXT("  Spec class: %s"), *Spec.Ability->GetClass()->GetName());
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
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation NavLoc;
	bool bIsHostile = false;                                                                                        
	if (HitActor)
	{
		if (HitActor->ActorHasTag("Enemy") || TargetingComponent->IsActorTargetPVPValid(HitActor, GetPawn()))
		{
			TargetingComponent->SetTarget(HitActor);
			StartAutoAttack();
			bIsHostile = true;
		}
	}                
	if (NavSys && NavSys->ProjectPointToNavigation(HitResult.Location, NavLoc))
	{
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, NavLoc.Location);
	}
	else if (HitActor)
	{
		UAIBlueprintHelperLibrary::SimpleMoveToActor(this, HitActor);
	}                                         
	if (!bIsHostile)
	{
		TargetingComponent->ClearTarget();                                                                          
		StopAutoAttack();
	}
}	


static void LogAbilityTargetData(int32 AbilityIndex, const FOnsetTargetData& Data)
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
	FOnsetTargetData Data = UAbilityTargetingLibrary::GetTargetData(TargetingComponent, GetPawn());
	LogAbilityTargetData(1, Data);
}

void AOnsetPlayerController::OnAbility2(const FInputActionValue& Value)
{
	FOnsetTargetData Data = UAbilityTargetingLibrary::GetTargetData(TargetingComponent, GetPawn());
	LogAbilityTargetData(2, Data);
}

void AOnsetPlayerController::OnAbility3(const FInputActionValue& Value)
{
	FOnsetTargetData Data = UAbilityTargetingLibrary::GetTargetData(TargetingComponent, GetPawn());
	LogAbilityTargetData(3, Data);
}

void AOnsetPlayerController::OnAbility4(const FInputActionValue& Value)
{
	FOnsetTargetData Data = UAbilityTargetingLibrary::GetTargetData(TargetingComponent, GetPawn());
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

void AOnsetPlayerController::Server_SetPvPEnabled_Implementation(bool bEnabled)                                 
{                                                                                                               
	AOnsetPlayerState* OnsetPlayerState = GetPlayerState<AOnsetPlayerState>();                                                
	if(OnsetPlayerState) {
		OnsetPlayerState->bIsPvPEnabled = bEnabled;
		UE_LOG(LogTemp, Warning, TEXT("PvP Mode %hs"), bEnabled ? "enabled" : "disabled");
	}
}   