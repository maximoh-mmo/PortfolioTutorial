// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/OnsetPlayerController.h"

#include "AbilitySystemComponent.h"
#include "EnhancedInputComponent.h"
#include "Multiplayer/OnsetGameModeBase.h"
#include "Player/CursorManager.h"
#include "Core/TargetingComponent.h"
#include "Data/OnsetPlayerDataTypes.h"
#include "EnhancedInputSubsystems.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Player/InteractionComponent.h"
#include "Core/OnsetBaseCharacter.h"
#include "UI/CharacterSelectWidget.h"
#include "Player/OnsetPlayerCharacter.h"
#include "Player/OnsetPlayerState.h"
#include "Subsystem/OnsetPlayerDataSubsystem.h"
#include "GAS/OnsetAttributeSet.h"
#include "Player/OnsetCheatManager.h"
#include "Player/OnsetPlayerAIController.h"
#include "Player/OnsetPlayerState.h"
#include "UI/GamepadCursorWidget.h"
#include "Kismet/GameplayStatics.h"

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

void AOnsetPlayerController::RequestSteamAuth()
{
	if (!GetLocalPlayer())
	{
		return;
	}

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get(STEAM_SUBSYSTEM);
	if (!Subsystem || !Subsystem->IsEnabled())
	{
		UE_LOG(LogSteamAuth, Log, TEXT("Steam not available — skipping auth (LAN mode)."));
		return;
	}

	IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface();
	if (!Identity.IsValid())
	{
		UE_LOG(LogSteamAuth, Warning, TEXT("Steam available but identity interface missing — skipping auth."));
		return;
	}

	const FString AuthTicket = Identity->GetAuthToken(0);
	if (AuthTicket.IsEmpty())
	{
		UE_LOG(LogSteamAuth, Error, TEXT("Steam auth ticket is empty — cannot authenticate."));
		return;
	}

	UE_LOG(LogSteamAuth, Log, TEXT("Auth ticket obtained (%d chars), sending to server."), AuthTicket.Len());
	Server_SendAuthTicket(AuthTicket);

	GetWorldTimerManager().SetTimer(AuthTimeoutTimerHandle, this, &AOnsetPlayerController::OnAuthTimeout, 10.0f, false);
}

void AOnsetPlayerController::OnAuthTimeout()
{
	UE_LOG(LogSteamAuth, Error, TEXT("Auth validation timed out — server did not respond within 10 seconds."));
}

void AOnsetPlayerController::ClearAuthTimeout()
{
	GetWorldTimerManager().ClearTimer(AuthTimeoutTimerHandle);
}

void AOnsetPlayerController::Client_AccountData_Implementation(const FOnsetAccountData& AccountData)
{
	UE_LOG(LogTemp, Log, TEXT("Client_AccountData: received %d slots"), AccountData.Slots.Num());

	if (CharacterSelectWidgetClass)
	{
		CharacterSelectWidget = CreateWidget<UCharacterSelectWidget>(this, CharacterSelectWidgetClass);
		if (CharacterSelectWidget)
		{
			CharacterSelectWidget->SetAccountData(AccountData);
			CharacterSelectWidget->AddToViewport(200);
		}
	}
}

void AOnsetPlayerController::Client_ClearAuthTimeout_Implementation()
{
	ClearAuthTimeout();
}

void AOnsetPlayerController::Server_SendAuthTicket_Implementation(const FString& AuthTicket)
{
	AOnsetGameModeBase* GM = GetWorld()->GetAuthGameMode<AOnsetGameModeBase>();
	if (GM)
	{
		GM->ValidateAuthTicket(this, AuthTicket);
	}
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

void AOnsetPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
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

	if (HasAuthority())
	{
		AutoCombatController = GetWorld()->SpawnActor<AOnsetPlayerAIController>(AOnsetPlayerAIController::StaticClass());
	}

	RequestSteamAuth();
	GetWorldTimerManager().SetTimer(IdleAutoCombatTimerHandle, this,
		&AOnsetPlayerController::EnableAutoCombat, IdleAutoCombatDelay * 4.0f, false);
}

void AOnsetPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInputComponent->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AOnsetPlayerController::OnMove);
		EnhancedInputComponent->BindAction(IA_Cursor, ETriggerEvent::Triggered, this, &AOnsetPlayerController::OnCursorMove);
		EnhancedInputComponent->BindAction(IA_Primary, ETriggerEvent::Started, this, &AOnsetPlayerController::OnPrimaryInteraction);
		EnhancedInputComponent->BindAction(IA_Cursor, ETriggerEvent::Completed, this, &AOnsetPlayerController::OnCursorMoveEnded);
		EnhancedInputComponent->BindAction(IA_Ability1, ETriggerEvent::Started, this, &AOnsetPlayerController::OnAbility1);
		EnhancedInputComponent->BindAction(IA_Ability2, ETriggerEvent::Started, this, &AOnsetPlayerController::OnAbility2);
		EnhancedInputComponent->BindAction(IA_Ability3, ETriggerEvent::Started, this, &AOnsetPlayerController::OnAbility3);
		EnhancedInputComponent->BindAction(IA_Ability4, ETriggerEvent::Started, this, &AOnsetPlayerController::OnAbility4);
		EnhancedInputComponent->BindAction(IA_PvPToggle, ETriggerEvent::Started, this, &AOnsetPlayerController::OnPvPToggleTriggered);
		
	}
}

void AOnsetPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	CachedPlayerPawn = InPawn;
	TargetingComponent = InPawn->FindComponentByClass<UTargetingComponent>();

	// After zone travel, restore saved character data onto the new pawn
	AOnsetPlayerState* PS = GetPlayerState<AOnsetPlayerState>();
	if (!PS || PS->SelectedCharacterSlot < 0) return;

	UOnsetPlayerDataSubsystem* DataSubsystem = GetWorld()->GetSubsystem<UOnsetPlayerDataSubsystem>();
	if (!DataSubsystem) return;

	FOnsetFullCharacterData CharData;
	if (!DataSubsystem->LoadCharacter(PS->PlayerPlatform, PS->PlayerPlatformID, PS->SelectedCharacterSlot, CharData))
	{
		UE_LOG(LogTemp, Warning, TEXT("OnPossess: failed to load character data for slot %d"), PS->SelectedCharacterSlot);
		return;
	}

	AOnsetPlayerCharacter* PlayerChar = Cast<AOnsetPlayerCharacter>(InPawn);
	if (!PlayerChar) return;

	// Only restore saved position on first login (no pending entry point).
	// Zone travel uses ChoosePlayerStart for placement instead.
	if (PS->PendingEntryPoint.IsEmpty())
	{
		PlayerChar->SetActorLocation(CharData.SavedPosition);
		PlayerChar->SetActorRotation(FRotator(0.0f, CharData.SavedRotationYaw, 0.0f));
	}

	if (PlayerChar->AttributeSet)
	{
		PlayerChar->AttributeSet->SetMaxHealth(CharData.SavedMaxHealth);
		PlayerChar->AttributeSet->SetHealth(CharData.SavedMaxHealth);
	}

	PlayerChar->GrantDefaultAbilities();

	UE_LOG(LogTemp, Log, TEXT("OnPossess: restored %s (slot %d) at %s"),
		*CharData.CharacterName, CharData.SlotIndex, *CharData.SavedPosition.ToString());
}

void AOnsetPlayerController::OnUnPossess()
{
	TargetingComponent = nullptr;
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

void AOnsetPlayerController::OnAutoAttackTick()
{
	AOnsetBaseCharacter* Self = GetPawn<AOnsetBaseCharacter>();
	if (!Self || !Self->AbilitySystemComponent || !Self->TargetingComponent || !Self->TargetingComponent->GetTarget() || !BasicAttackAbility)
	{
		StopAutoAttack();
		return;
	}
	Self->AbilitySystemComponent->TryActivateAbilityByClass(BasicAttackAbility);
}

void AOnsetPlayerController::OnMove(const FInputActionValue& Value)
{
	Server_DisableAutoCombat();
	ResetIdleTimer();
	FVector2D MovementVector = Value.Get<FVector2D>();
	if (MovementVector.IsZero()) return;	
	StopMovement();
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) ControlledPawn = CachedPlayerPawn;
	if (ControlledPawn)
	{
		ControlledPawn->AddMovementInput(ControlledPawn->GetActorForwardVector(), MovementVector.Y);
		ControlledPawn->AddMovementInput(ControlledPawn->GetActorRightVector(), MovementVector.X);
	}
}

void AOnsetPlayerController::OnCursorMove(const FInputActionValue& Value)
{
	bShowMouseCursor = false;
	const FVector2D CursorDelta = Value.Get<FVector2D>();
	if (CursorDelta.IsZero()) return;
	if (const UWorld* World = GetWorld())
	{
		CursorManager->AddGamepadCursorDelta(CursorDelta, World->GetDeltaSeconds());
		CursorManager->ClampToViewport();
		FVector2D ScreenPos;
		if (CursorManager->GetCursorPosition(ScreenPos))
		{
			if (GamepadCursorWidget)
			{
				GamepadCursorWidget->SetCursorPosition(ScreenPos);
				GamepadCursorWidget->ShowCursor();
			}
			GetWorldTimerManager().ClearTimer(CursorIdleTimerHandle);
		}
	}
}

void AOnsetPlayerController::OnCursorMoveEnded(const FInputActionValue& Value)
{
	if (UWorld* World = GetWorld())
		World->GetTimerManager().SetTimer(CursorIdleTimerHandle, this, &AOnsetPlayerController::HideGamepadCursor,
		                                  CursorIdleDelay, false);
}

void AOnsetPlayerController::OnPrimaryInteraction(const FInputActionValue& Value)
{
	ResetIdleTimer();
	FVector2D ScreenPos;
	if (!CursorManager->GetCursorPosition(ScreenPos)) return;

	FVector WorldOrigin, WorldDirection;
	if (!UGameplayStatics::DeprojectScreenToWorld(this, ScreenPos, WorldOrigin, WorldDirection))
		return;

	const float TraceDistance = 10000.0f;
	const FVector TraceEnd = WorldOrigin + WorldDirection * TraceDistance;
	AActor* HitActor = nullptr;
	FVector HitLocation = FVector::ZeroVector;

	// First: object trace for Pawn channel (enemy characters, players)
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;

	FHitResult PawnHit;
	if (GetWorld()->LineTraceSingleByObjectType(PawnHit, WorldOrigin, TraceEnd, ObjectParams, QueryParams))
	{
		HitActor = PawnHit.GetActor();
		HitLocation = PawnHit.Location;
	}
	else
	{
		// Fall back to Visibility trace for world surfaces (terrain, walls)
		FHitResult VisHit;
		if (GetHitResultAtScreenPosition(ScreenPos, ECC_Visibility, false, VisHit))
		{
			HitActor = VisHit.GetActor();
			HitLocation = VisHit.Location;
		}
	}

	UE_LOG(LogGamepad, Log, TEXT("Click: HitActor=%s"), HitActor ? *HitActor->GetName() : TEXT("null"));
	Server_ProcessPrimaryInteraction(HitActor, HitLocation);
}

void AOnsetPlayerController::Server_ProcessPrimaryInteraction_Implementation(AActor* HitActor, FVector HitLocation)
{
	if (!InteractionComponent)
	{
		return;
	}
	
	if (HitActor && !IsValid(HitActor))
	{
		HitActor = nullptr;
	}
	
	// Process targeting first (may change bAutoCombatEnabled)
	InteractionComponent->ProcessPrimaryInteraction(HitActor, HitLocation);
	
	FVector MoveTarget = InteractionComponent->GetPendingMoveTarget();
	if (MoveTarget != FVector::ZeroVector)
	{
		if (!bAutoCombatEnabled)
		{
			EnableAutoCombat();
		}
		
		if (AutoCombatController && AutoCombatController->GetPawn())
		{
			AutoCombatController->StopStateTree();
			AutoCombatController->MoveToLocation(MoveTarget);
		}
	}
}	

void AOnsetPlayerController::OnAbility1(const FInputActionValue& Value)
{
	Server_DisableAutoCombat();
	ResetIdleTimer();
}

void AOnsetPlayerController::OnAbility2(const FInputActionValue& Value)
{	
	Server_DisableAutoCombat();
	ResetIdleTimer();
}

void AOnsetPlayerController::OnAbility3(const FInputActionValue& Value)
{
	Server_DisableAutoCombat();
	ResetIdleTimer();
}

void AOnsetPlayerController::OnAbility4(const FInputActionValue& Value)
{
	Server_DisableAutoCombat();
	ResetIdleTimer();
}

void AOnsetPlayerController::InjectAbilityInput(const int32 AbilityIndex, const bool bPressed) const
{
	// Route to the right action via the Enhanced Input subsystem                                               
	UEnhancedInputLocalPlayerSubsystem* Subsystem =                                                             
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());                       
	if (!Subsystem) return;

	UInputAction* Action;                                                                             
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
	const AOnsetPlayerState* OnsetPlayerState = GetPlayerState<AOnsetPlayerState>();                                                
	if (!OnsetPlayerState) return;                                                                                            
	Server_SetPvPEnabled(!OnsetPlayerState->bIsPvPEnabled);                                                                   
}


void AOnsetPlayerController::Server_SetPvPEnabled_Implementation(const bool bEnabled)                                 
{
	if (AOnsetPlayerState* OnsetPlayerState = GetPlayerState<AOnsetPlayerState>())
	{
		OnsetPlayerState->bIsPvPEnabled = bEnabled;
	}
}

void AOnsetPlayerController::Server_DisableAutoCombat_Implementation()
{
	DisableAutoCombat();
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
	if (AIPawn)
	{
		Possess(AIPawn);
	}
	bAutoCombatEnabled = false;
	ResetIdleTimer();
}

const AController* AOnsetPlayerController::GetActiveController() const
{
	AController* AutoController = AutoCombatController;
	return bAutoCombatEnabled ? AutoController : this;
}

void AOnsetPlayerController::ResetIdleTimer()
{
	bIdleTimerInitialized = true;
	GetWorldTimerManager().ClearTimer(IdleAutoCombatTimerHandle);
	if (IdleAutoCombatDelay > 0.0f)
	{	
		GetWorldTimerManager().SetTimer(IdleAutoCombatTimerHandle, this,
			&AOnsetPlayerController::EnableAutoCombat, IdleAutoCombatDelay, false);
	}
}

void AOnsetPlayerController::Client_CharacterData_Implementation(const FOnsetFullCharacterData& CharacterData)
{
	UE_LOG(LogTemp, Log, TEXT("Client_CharacterData: received '%s' (slot %d, lvl %d)"),
		*CharacterData.CharacterName, CharacterData.SlotIndex, CharacterData.Level);
}

void AOnsetPlayerController::Client_SaveComplete_Implementation(bool bSuccess)
{
	UE_LOG(LogTemp, Log, TEXT("Client_SaveComplete: %s"), bSuccess ? TEXT("success") : TEXT("failed"));
}

void AOnsetPlayerController::Server_SelectCharacter_Implementation(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex > 2) return;

	AOnsetPlayerState* PS = GetPlayerState<AOnsetPlayerState>();
	if (!PS) return;

	UOnsetPlayerDataSubsystem* DataSubsystem = GetWorld()->GetSubsystem<UOnsetPlayerDataSubsystem>();
	if (!DataSubsystem) return;

	FOnsetFullCharacterData CharData;
	if (!DataSubsystem->LoadCharacter(PS->PlayerPlatform, PS->PlayerPlatformID, SlotIndex, CharData))
	{
		UE_LOG(LogTemp, Error, TEXT("Server_SelectCharacter: failed to load slot %d"), SlotIndex);
		return;
	}

	PS->SelectedCharacterSlot = SlotIndex;

	AOnsetPlayerCharacter* PlayerCharacter = Cast<AOnsetPlayerCharacter>(GetPawn());
	if (!PlayerCharacter)
	{
		UClass* PawnClass = AOnsetPlayerCharacter::StaticClass();
		if (const AGameModeBase* GM = GetWorld()->GetAuthGameMode())
			PawnClass = GM->DefaultPawnClass;

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		PlayerCharacter = GetWorld()->SpawnActor<AOnsetPlayerCharacter>(PawnClass, CharData.SavedPosition, FRotator(0.0, CharData.SavedRotationYaw, 0.0), SpawnParams);
		if (PlayerCharacter)
		{
			Possess(PlayerCharacter);
			PlayerCharacter->InitAbilityActorInfo();
		}
	}

	if (PlayerCharacter)
	{
		PlayerCharacter->SetActorLocation(CharData.SavedPosition);
		PlayerCharacter->SetActorRotation(FRotator(0.0f, CharData.SavedRotationYaw, 0.0f));

		if (PlayerCharacter->AttributeSet)
		{
			PlayerCharacter->AttributeSet->SetMaxHealth(CharData.SavedMaxHealth);
			PlayerCharacter->AttributeSet->SetHealth(CharData.SavedMaxHealth);
		}

		PlayerCharacter->GrantDefaultAbilities();
	}

	Client_CharacterData(CharData);

	UE_LOG(LogTemp, Log, TEXT("Server_SelectCharacter: player %s selected slot %d (%s)"),
		*PS->GetPlayerName(), SlotIndex, *CharData.CharacterName);

	// Travel all players to the game world
	GetWorld()->ServerTravel(TEXT("DemoLevel?game=/Game/OnsetGameMode.OnsetGameMode_C"));
}

void AOnsetPlayerController::Server_CreateCharacter_Implementation(int32 SlotIndex, const FString& CharacterName)
{
	if (SlotIndex < 0 || SlotIndex > 2 || CharacterName.IsEmpty()) return;

	AOnsetPlayerState* PS = GetPlayerState<AOnsetPlayerState>();
	if (!PS) return;

	UOnsetPlayerDataSubsystem* DataSubsystem = GetWorld()->GetSubsystem<UOnsetPlayerDataSubsystem>();
	if (!DataSubsystem) return;

	FOnsetAccountData AccountData;
	if (DataSubsystem->LoadAccount(PS->PlayerPlatform, PS->PlayerPlatformID, AccountData))
	{
		for (const auto& Slot : AccountData.Slots)
		{
			if (Slot.SlotIndex == SlotIndex && Slot.bOccupied)
			{
				UE_LOG(LogTemp, Warning, TEXT("Server_CreateCharacter: slot %d already occupied"), SlotIndex);
				return;
			}
		}
	}

	FOnsetFullCharacterData NewChar;
	NewChar.SlotIndex = SlotIndex;
	NewChar.CharacterName = CharacterName;
	NewChar.Level = 1;
	NewChar.Experience = 0;
	NewChar.SavedMaxHealth = 100.0f;
	NewChar.SavedPosition = FVector(0.0f, 0.0f, 150.0f);
	NewChar.SavedRotationYaw = 0.0f;
	NewChar.CurrentZone = TEXT("");
	NewChar.InventoryJSON = TEXT("{}");
	NewChar.EquipmentJSON = TEXT("{}");
	NewChar.QuestsJSON = TEXT("{}");

	if (DataSubsystem->SaveCharacter(PS->PlayerPlatform, PS->PlayerPlatformID, NewChar))
	{
		UE_LOG(LogTemp, Log, TEXT("Server_CreateCharacter: created '%s' in slot %d"), *CharacterName, SlotIndex);
		Server_SelectCharacter(SlotIndex);
	}
}

void AOnsetPlayerController::Server_SaveCharacter_Implementation()
{
	AOnsetPlayerState* PS = GetPlayerState<AOnsetPlayerState>();
	if (!PS || PS->SelectedCharacterSlot < 0) return;

	AOnsetPlayerCharacter* PlayerCharacter = Cast<AOnsetPlayerCharacter>(GetPawn());
	if (!PlayerCharacter) return;

	UOnsetPlayerDataSubsystem* DataSubsystem = GetWorld()->GetSubsystem<UOnsetPlayerDataSubsystem>();
	if (!DataSubsystem) return;

	FOnsetFullCharacterData CharData;
	CharData.SlotIndex = PS->SelectedCharacterSlot;
	CharData.CharacterName = PS->GetPlayerName();
	CharData.Level = 1;
	CharData.Experience = 0;

	if (PlayerCharacter->AttributeSet)
	{
		CharData.SavedMaxHealth = PlayerCharacter->AttributeSet->GetMaxHealth();
	}
	CharData.SavedPosition = PlayerCharacter->GetActorLocation();
	CharData.SavedRotationYaw = PlayerCharacter->GetActorRotation().Yaw;
	CharData.CurrentZone = GetWorld()->GetMapName();
	CharData.InventoryJSON = TEXT("{}");
	CharData.EquipmentJSON = TEXT("{}");
	CharData.QuestsJSON = TEXT("{}");

	bool bSuccess = DataSubsystem->SaveCharacter(PS->PlayerPlatform, PS->PlayerPlatformID, CharData);
	Client_SaveComplete(bSuccess);
}

void AOnsetPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (EndPlayReason == EEndPlayReason::Destroyed || EndPlayReason == EEndPlayReason::RemovedFromWorld)
	{
		AOnsetPlayerState* PS = GetPlayerState<AOnsetPlayerState>();
		if (PS && PS->SelectedCharacterSlot >= 0)
		{
			AOnsetPlayerCharacter* PlayerCharacter = Cast<AOnsetPlayerCharacter>(GetPawn());
			if (PlayerCharacter)
			{
				UOnsetPlayerDataSubsystem* DataSubsystem = GetWorld()->GetSubsystem<UOnsetPlayerDataSubsystem>();
				if (DataSubsystem)
				{
					FOnsetFullCharacterData CharData;
					CharData.SlotIndex = PS->SelectedCharacterSlot;
					CharData.SavedPosition = PlayerCharacter->GetActorLocation();
					CharData.SavedRotationYaw = PlayerCharacter->GetActorRotation().Yaw;
					CharData.CurrentZone = GetWorld()->GetMapName();
					if (PlayerCharacter->AttributeSet)
					{
						CharData.SavedMaxHealth = PlayerCharacter->AttributeSet->GetMaxHealth();
					}
					CharData.InventoryJSON = TEXT("{}");
					CharData.EquipmentJSON = TEXT("{}");
					CharData.QuestsJSON = TEXT("{}");
					DataSubsystem->SaveCharacter(PS->PlayerPlatform, PS->PlayerPlatformID, CharData);
				}
			}
		}
	}
	Super::EndPlay(EndPlayReason);
}