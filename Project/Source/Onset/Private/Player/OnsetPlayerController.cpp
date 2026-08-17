// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/OnsetPlayerController.h"

#include "AbilitySystemComponent.h"
#include "EnhancedInputComponent.h"
#include "Game/OnsetGameModeBase.h"
#include "Subsystem/OnsetAuthSubsystem.h"
#include "Player/CursorManager.h"
#include "Core/TargetingComponent.h"
#include "OnsetPlayerDataTypes.h"
#include "EnhancedInputSubsystems.h"
#include "NavigationSystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "TimerManager.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Player/InteractionComponent.h"
#include "Core/OnsetBaseCharacter.h"
#include "Engine/GameInstance.h"
#include "Player/OnsetPlayerCharacter.h"
#include "Player/OnsetPlayerState.h"
#include "Subsystem/OnsetPlayerDataSubsystem.h"
#include "GAS/OnsetAttributeSet.h"
#include "Player/OnsetCheatManager.h"
#include "Player/OnsetPlayerAIController.h"
#include "UI/GamepadCursorWidget.h"
#include "UI/HUDWidget.h"
#include "Subsystem/OnsetUISubsystem.h"
#include "UI/OnsetRootLayout.h"
#include "UI/OnsetScreenBase.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Engine/DataTable.h"
#include "Data/OnsetClassInfoTypes.h"
#include "Inventory/UOnsetInventoryComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

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

	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld(), STEAM_SUBSYSTEM);
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

void AOnsetPlayerController::Client_AccountData_Implementation(const FOnsetAccountData& InAccountData)
{
	CachedAccountData = InAccountData;
	bShowMouseCursor = true;
	SetInputMode(FInputModeGameAndUI());
	OnAccountDataChanged.Broadcast();
	BP_OnAccountDataUpdated();

	// Test harness: -AutoPlaySlot=N auto-enters an occupied slot (no menu clicks).
	int32 AutoSlot = -1;
	if (FParse::Value(FCommandLine::Get(), TEXT("AutoPlaySlot="), AutoSlot) && IsLocalController())
	{
		if (AutoSlot < 0 || AutoSlot >= CachedAccountData.Slots.Num() || !CachedAccountData.Slots[AutoSlot].bOccupied)
		{
			for (int32 SlotIdx = 0; SlotIdx < CachedAccountData.Slots.Num(); ++SlotIdx)
			{
				if (CachedAccountData.Slots[SlotIdx].bOccupied)
				{
					AutoSlot = SlotIdx;
					break;
				}
			}
		}
		if (AutoSlot >= 0 && AutoSlot < CachedAccountData.Slots.Num() && CachedAccountData.Slots[AutoSlot].bOccupied)
		{
			AutoPlaySlotIndex = AutoSlot;
			GetWorldTimerManager().SetTimer(AutoPlayTimerHandle, this, &AOnsetPlayerController::AutoPlaySelectCharacter, 0.5f, false);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Client_AccountData: AutoPlaySlot requested but no occupied slot found (slots=%d)"), CachedAccountData.Slots.Num());
		}
	}
}

void AOnsetPlayerController::AutoPlaySelectCharacter()
{
	if (AutoPlaySlotIndex >= 0 && AutoPlaySlotIndex < CachedAccountData.Slots.Num() && CachedAccountData.Slots[AutoPlaySlotIndex].bOccupied)
	{
		UE_LOG(LogTemp, Warning, TEXT("AutoPlay: selecting character slot %d"), AutoPlaySlotIndex);
		Server_SelectCharacter(AutoPlaySlotIndex);
	}
}

void AOnsetPlayerController::Client_ShowMainMenuUI_Implementation(
	TSubclassOf<UOnsetRootLayout> RootLayoutClass,
	TSubclassOf<UOnsetScreenBase> MainMenuClass)
{
	auto* UI = GetGameInstance()->GetSubsystem<UOnsetUISubsystem>();
	if (!UI) return;
	if (RootLayoutClass) UI->InitializeRootLayout(RootLayoutClass);
	if (MainMenuClass)   UI->PushScreen(EOnsetUILayer::Game, MainMenuClass);
}

void AOnsetPlayerController::Client_CleanupUI_Implementation()
{
	auto* UI = GetGameInstance()->GetSubsystem<UOnsetUISubsystem>();
	if (UI) UI->CleanupUI();
}

void AOnsetPlayerController::Client_ClearAuthTimeout_Implementation()
{
	ClearAuthTimeout();
}

void AOnsetPlayerController::Server_SendAuthTicket_Implementation(const FString& AuthTicket)
{
	UOnsetAuthSubsystem* Auth = GetWorld()->GetSubsystem<UOnsetAuthSubsystem>();
	if (Auth)
	{
		Auth->ValidateAuthTicket(this, AuthTicket);
	}
}

void AOnsetPlayerController::StartAutoAttack()
{
	// Ensure we have a valid world context before setting the timer. The interval is a
	// short echo poll: the basic-attack cooldown GE is the actual attack-rate gate.
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

	const AOnsetPlayerState* PossessPS = GetPlayerState<AOnsetPlayerState>();
	UE_LOG(LogTemp, Warning, TEXT("OnPossess: local=%d slot=%d pawn=%s"),
		IsLocalController(), PossessPS ? PossessPS->SelectedCharacterSlot : -1, *GetNameSafe(InPawn));

	// Re-acquiring a pawn means auto-combat handed control back (or the player just
	// spawned); nudge the HUD toggle so it always mirrors the real state, even when
	// the owning client would otherwise only hear about it via replication.
	if (AOnsetPlayerState* PS = GetPlayerState<AOnsetPlayerState>())
	{
		PS->OnPlayerSettingsChanged.Broadcast();
	}

	// Re-possessing a pawn this controller already owned this session (e.g. when
	// the auto-combat AI hands control back) must NOT re-apply the saved state.
	// Otherwise players could toggle auto-combat to warp back to their last save
	// position or refill to full health.
	const bool bIsRePossess = (InPawn == CachedPlayerPawn);
	CachedPlayerPawn = InPawn;
	TargetingComponent = InPawn->FindComponentByClass<UTargetingComponent>();

	if (bIsRePossess)
	{
		UE_LOG(LogTemp, Log, TEXT("OnPossess: re-possessing %s, skipping saved-state restore"), *GetNameSafe(InPawn));
		return;
	}

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

	// Apply the character's build (class base stats + equipped loadout) and heal to full.
	PlayerChar->ApplyCharacterBuild(CharData.CharacterClass, CharData.EquipmentJSON);
	PlayerChar->DeserializeInventoryJSON(CharData.InventoryJSON);
	if (PlayerChar->AttributeSet)
	{
		PlayerChar->AttributeSet->SetHealth(PlayerChar->AttributeSet->GetMaxHealth());
	}

	PlayerChar->GrantDefaultAbilities();

	// Apply appearance preset if JSON is available
	if (!CharData.AppearanceJSON.IsEmpty())
	{
		TSharedPtr<FJsonObject> AppObj;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(CharData.AppearanceJSON);
		if (FJsonSerializer::Deserialize(Reader, AppObj) && AppObj.IsValid())
		{
			int32 PresetIndex = AppObj->GetIntegerField(TEXT("PresetIndex"));
			BP_ApplyAppearancePreset(PlayerChar, PresetIndex);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("OnPossess: restored %s (slot %d) at %s"),
		*CharData.CharacterName, CharData.SlotIndex, *CharData.SavedPosition.ToString());
}

void AOnsetPlayerController::CreateHUD(APawn* InPawn)
{
	if (!IsLocalController() || HUDWidget)
	{
		return;
	}

	if (!HUDWidgetClass)
	{
		HUDWidgetClass = UHUDWidget::StaticClass();
	}

	UE_LOG(LogTemp, Warning, TEXT("CreateHUD: creating HUD with class %s"), HUDWidgetClass ? *HUDWidgetClass->GetName() : TEXT("NULL"));
	HUDWidget = CreateWidget<UHUDWidget>(this, HUDWidgetClass);
	if (HUDWidget)
	{
		HUDWidget->AddToViewport(1);
		if (AOnsetBaseCharacter* PawnChar = Cast<AOnsetBaseCharacter>(InPawn))
		{
			HUDWidget->BindToPlayer(this, PawnChar);
		}
	}
}

void AOnsetPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();

	if (!IsLocalController()) return;

	APawn* NewPawn = GetPawn();
	if (!NewPawn) return;

	// The client now possesses a pawn — dismiss the loading screen and build the HUD.
	if (UOnsetUISubsystem* UI = GetGameInstance()->GetSubsystem<UOnsetUISubsystem>())
	{
		UI->HideLoadingScreen();
	}

	CreateHUD(NewPawn);
	Server_OnClientPossessed();
}

void AOnsetPlayerController::OnUnPossess()
{
	TargetingComponent = nullptr;

	// The auto-combat AI took the pawn; nudge the HUD toggle so it mirrors the state.
	if (AOnsetPlayerState* PS = GetPlayerState<AOnsetPlayerState>())
	{
		PS->OnPlayerSettingsChanged.Broadcast();
	}
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
		const FRotator CameraYaw(0.0f, GetControlRotation().Yaw, 0.0f);
		const FVector Forward = FRotationMatrix(CameraYaw).GetUnitAxis(EAxis::X);
		const FVector Right   = FRotationMatrix(CameraYaw).GetUnitAxis(EAxis::Y);
		ControlledPawn->AddMovementInput(Forward, MovementVector.Y);
		ControlledPawn->AddMovementInput(Right, MovementVector.X);
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

	// Process targeting first.
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

void AOnsetPlayerController::OnsetGrantItem(const FString& RowName)
{
	if (RowName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("OnsetGrantItem: empty row name"));
		return;
	}
	Server_GrantItem(RowName);
}

void AOnsetPlayerController::Server_GrantItem_Implementation(const FString& RowName)
{
	if (RowName.IsEmpty())
	{
		return;
	}
	AOnsetPlayerCharacter* PlayerCharacter = Cast<AOnsetPlayerCharacter>(GetPawn());
	if (!PlayerCharacter || !PlayerCharacter->InventoryComponent)
	{
		return;
	}
	PlayerCharacter->InventoryComponent->AddItem(FName(*RowName));
	UE_LOG(LogTemp, Log, TEXT("OnsetGrantItem: granted '%s'"), *RowName);
}

void AOnsetPlayerController::OnAbility1(const FInputActionValue& Value)
{
	Server_DisableAutoCombat();
	ResetIdleTimer();
	AOnsetBaseCharacter* Self = GetPawn<AOnsetBaseCharacter>();
	if (Self && Self->AbilitySystemComponent)
	{
		Self->AbilitySystemComponent->AbilityLocalInputPressed(1); // Input ID 1 = slot 1
		Self->AbilitySystemComponent->AbilityLocalInputReleased(1);
	}
}

void AOnsetPlayerController::OnAbility2(const FInputActionValue& Value)
{
	Server_DisableAutoCombat();
	ResetIdleTimer();
	AOnsetBaseCharacter* Self = GetPawn<AOnsetBaseCharacter>();
	if (Self && Self->AbilitySystemComponent)
	{
		Self->AbilitySystemComponent->AbilityLocalInputPressed(2); // Input ID 2 = slot 2
		Self->AbilitySystemComponent->AbilityLocalInputReleased(2);
	}
}

void AOnsetPlayerController::OnAbility3(const FInputActionValue& Value)
{
	Server_DisableAutoCombat();
	ResetIdleTimer();
	AOnsetBaseCharacter* Self = GetPawn<AOnsetBaseCharacter>();
	if (Self && Self->AbilitySystemComponent)
	{
		Self->AbilitySystemComponent->AbilityLocalInputPressed(3); // Input ID 3 = slot 3
		Self->AbilitySystemComponent->AbilityLocalInputReleased(3);
	}
}

void AOnsetPlayerController::OnAbility4(const FInputActionValue& Value)
{
	Server_DisableAutoCombat();
	ResetIdleTimer();
	AOnsetBaseCharacter* Self = GetPawn<AOnsetBaseCharacter>();
	if (Self && Self->AbilitySystemComponent)
	{
		Self->AbilitySystemComponent->AbilityLocalInputPressed(4); // Input ID 4 = slot 4
		Self->AbilitySystemComponent->AbilityLocalInputReleased(4);
	}
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

void AOnsetPlayerController::Server_SetAutoCombatEnabled_Implementation(bool bEnabled)
{
	if (bEnabled)
	{
		EnableAutoCombat();
	}
	else
	{
		DisableAutoCombat();
	}
}

void AOnsetPlayerController::Server_SetContinueOnDisconnect_Implementation(bool bEnabled)
{
	if (AOnsetPlayerState* PS = GetPlayerState<AOnsetPlayerState>())
	{
		PS->bContinueOnDisconnect = bEnabled;
		PS->OnPlayerSettingsChanged.Broadcast();
	}
}

void AOnsetPlayerController::SetAutoCombatEnabled(bool bEnabled)
{
	if (HasAuthority())
	{
		Server_SetAutoCombatEnabled_Implementation(bEnabled);
	}
	else
	{
		Server_SetAutoCombatEnabled(bEnabled);
	}
}

void AOnsetPlayerController::SetContinueOnDisconnect(bool bEnabled)
{
	if (HasAuthority())
	{
		Server_SetContinueOnDisconnect_Implementation(bEnabled);
	}
	else
	{
		Server_SetContinueOnDisconnect(bEnabled);
	}
}

AOnsetPlayerAIController* AOnsetPlayerController::GetAutoCombatController() const
{
	return AutoCombatController;
}

void AOnsetPlayerController::Server_OnClientPossessed_Implementation()
{
	ResetIdleTimer();
}

void AOnsetPlayerController::EnableAutoCombat()
{
	if (!AutoCombatController || bAutoCombatEnabled) return;
	if (APawn* MyPawn = GetPawn())
	{
		AutoCombatController->Possess(MyPawn);
		UnPossess();
		bAutoCombatEnabled = true;
		if (AOnsetPlayerState* PS = GetPlayerState<AOnsetPlayerState>())
		{
			PS->bAutoplayEnabled = true;
			// Server-side mutation doesn't trigger OnRep locally (standalone/listen
			// server), so notify the owning client's HUD directly. Remote clients are
			// still covered by the replicated OnRep.
			PS->OnPlayerSettingsChanged.Broadcast();
		}
		UE_LOG(LogTemp, Warning, TEXT("EnableAutoCombat: AI now controls %s (bAutoplayEnabled=%d)"),
			*GetNameSafe(MyPawn), bAutoCombatEnabled);
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
	if (AOnsetPlayerState* PS = GetPlayerState<AOnsetPlayerState>())
	{
		PS->bAutoplayEnabled = false;
		// See EnableAutoCombat: keep the owning client's HUD in sync on standalone/listen server.
		PS->OnPlayerSettingsChanged.Broadcast();
	}
	UE_LOG(LogTemp, Warning, TEXT("DisableAutoCombat: player regained control of %s (bAutoplayEnabled=%d)"),
		*GetNameSafe(AIPawn), bAutoCombatEnabled);
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

	UOnsetAuthSubsystem* Auth = GetWorld()->GetSubsystem<UOnsetAuthSubsystem>();
	if (Auth && Auth->GetAuthMode() == EOnsetAuthMode::Direct)
	{
		FString Token = Auth->GenerateToken(PS->PlayerPlatform, PS->PlayerPlatformID, SlotIndex);
		if (!Token.IsEmpty())
		{
			FString GameServerIP = TEXT("127.0.0.1");
			FString GameServerPort = TEXT("7777");
			GConfig->GetString(TEXT("Onset.Auth"), TEXT("GameServerIP"), GameServerIP, GEngineIni);
			GConfig->GetString(TEXT("Onset.Auth"), TEXT("GameServerPort"), GameServerPort, GEngineIni);
			Client_TravelToGameServer(GameServerIP, GameServerPort, Token);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Server_SelectCharacter: failed to generate travel token"));
		}
	}
	else
	{
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

			// Apply the character's build (class base stats + equipped loadout) and heal to full.
			PlayerCharacter->ApplyCharacterBuild(CharData.CharacterClass, CharData.EquipmentJSON);
			PlayerCharacter->DeserializeInventoryJSON(CharData.InventoryJSON);
			if (PlayerCharacter->AttributeSet)
			{
				PlayerCharacter->AttributeSet->SetHealth(PlayerCharacter->AttributeSet->GetMaxHealth());
			}

			PlayerCharacter->GrantDefaultAbilities();
		}
	}

	Client_CharacterData(CharData);

	UE_LOG(LogTemp, Log, TEXT("Server_SelectCharacter: player %s selected slot %d (%s)"),
		*PS->GetPlayerName(), SlotIndex, *CharData.CharacterName);
}

void AOnsetPlayerController::Server_CreateCharacter_Implementation(int32 SlotIndex, const FString& CharacterName, EOnsetCharacterClass CharacterClass, int32 AppearancePresetIndex)
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

	float ClassMaxHealth = 100.0f;
	{
		FString ClassTablePath;
		GConfig->GetString(TEXT("Onset.Gameplay"), TEXT("ClassDataTable"), ClassTablePath, GEngineIni);
		if (!ClassTablePath.IsEmpty())
		{
			UDataTable* ClassTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *ClassTablePath));
			if (ClassTable)
			{
				
				FName RowName = *UEnum::GetDisplayValueAsText(CharacterClass).ToString();
				FOnsetCharacterClassInfo* Row = ClassTable->FindRow<FOnsetCharacterClassInfo>(RowName, nullptr);
				if (Row)
				{
					ClassMaxHealth = Row->StartingMaxHealth;
				}
			}
		}
	}

	FOnsetCharacterAppearance Appearance;
	Appearance.PresetIndex = FMath::Clamp(AppearancePresetIndex, 0, 255);
	FString AppearanceJSON;
	{
		TSharedPtr<FJsonObject> AppObj = MakeShareable(new FJsonObject);
		AppObj->SetNumberField(TEXT("PresetIndex"), Appearance.PresetIndex);
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&AppearanceJSON);
		FJsonSerializer::Serialize(AppObj.ToSharedRef(), Writer);
	}

	FOnsetFullCharacterData NewChar;
	NewChar.SlotIndex = SlotIndex;
	NewChar.CharacterName = CharacterName;
	NewChar.CharacterClass = CharacterClass;
	NewChar.AppearanceJSON = AppearanceJSON;
	NewChar.Level = 1;
	NewChar.Experience = 0;
	NewChar.SavedMaxHealth = ClassMaxHealth;
	NewChar.SavedPosition = FVector(0.0f, 0.0f, 150.0f);
	NewChar.SavedRotationYaw = 0.0f;
	NewChar.CurrentZone = TEXT("");
	NewChar.InventoryJSON = TEXT("{}");
	NewChar.EquipmentJSON = TEXT("{}");
	NewChar.QuestsJSON = TEXT("{}");

	if (DataSubsystem->SaveCharacter(PS->PlayerPlatform, PS->PlayerPlatformID, NewChar))
	{
		UE_LOG(LogTemp, Log, TEXT("Server_CreateCharacter: created '%s' (class=%d) in slot %d, preset %d"),
			*CharacterName, static_cast<int32>(CharacterClass), SlotIndex, AppearancePresetIndex);

		// Send refreshed account data so the client cache is up to date.
		FOnsetAccountData RefreshedAccount;
		if (DataSubsystem->LoadAccount(PS->PlayerPlatform, PS->PlayerPlatformID, RefreshedAccount))
		{
			Client_AccountData(RefreshedAccount);
		}

		// Enter the world with the newly created character.
		Server_SelectCharacter(SlotIndex);
	}
}

void AOnsetPlayerController::Server_DeleteCharacter_Implementation(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex > 2) return;

	AOnsetPlayerState* PS = GetPlayerState<AOnsetPlayerState>();
	if (!PS) return;

	UOnsetPlayerDataSubsystem* DataSubsystem = GetWorld()->GetSubsystem<UOnsetPlayerDataSubsystem>();
	if (!DataSubsystem) return;

	if (DataSubsystem->DeleteCharacter(PS->PlayerPlatform, PS->PlayerPlatformID, SlotIndex))
	{
		UE_LOG(LogTemp, Log, TEXT("Server_DeleteCharacter: deleted slot %d"), SlotIndex);
	}

	// Send refreshed account data
	FOnsetAccountData RefreshedAccount;
	if (DataSubsystem->LoadAccount(PS->PlayerPlatform, PS->PlayerPlatformID, RefreshedAccount))
	{
		Client_AccountData(RefreshedAccount);
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

	if (PlayerCharacter->AttributeSet)
	{
		CharData.SavedMaxHealth = PlayerCharacter->AttributeSet->GetMaxHealth();
	}
	CharData.SavedPosition = PlayerCharacter->GetActorLocation();
	CharData.SavedRotationYaw = PlayerCharacter->GetActorRotation().Yaw;
	CharData.CurrentZone = GetWorld()->GetMapName();
	CharData.InventoryJSON = PlayerCharacter->SerializeInventoryJSON();
	CharData.EquipmentJSON = PlayerCharacter->SerializeEquipmentJSON();
	CharData.QuestsJSON = TEXT("{}");

	bool bSuccess = DataSubsystem->SaveCharacterPreservingIdentity(PS->PlayerPlatform, PS->PlayerPlatformID, CharData);
	Client_SaveComplete(bSuccess);
}

void AOnsetPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (EndPlayReason == EEndPlayReason::Destroyed || EndPlayReason == EEndPlayReason::RemovedFromWorld)
	{
		SaveCurrentCharacter(GetPawn());
	}
	Super::EndPlay(EndPlayReason);
}

bool AOnsetPlayerController::SaveCurrentCharacter(APawn* InPawn)
{
	// Exactly-once: disconnect triggers Logout -> PawnLeavingGame -> EndPlay, all of which
	// funnel here. Only the first call flushes, avoiding duplicate writes to the data store.
	if (bCharacterDataSaved)
	{
		return false;
	}

	AOnsetPlayerState* PS = GetPlayerState<AOnsetPlayerState>();
	if (!PS || PS->SelectedCharacterSlot < 0) return false;

	AOnsetPlayerCharacter* PlayerCharacter = Cast<AOnsetPlayerCharacter>(InPawn);
	if (!PlayerCharacter) return false;

	UOnsetPlayerDataSubsystem* DataSubsystem = GetWorld()->GetSubsystem<UOnsetPlayerDataSubsystem>();
	if (!DataSubsystem) return false;

	FOnsetFullCharacterData CharData;
	CharData.SlotIndex = PS->SelectedCharacterSlot;
	CharData.SavedPosition = PlayerCharacter->GetActorLocation();
	CharData.SavedRotationYaw = PlayerCharacter->GetActorRotation().Yaw;
	CharData.CurrentZone = GetWorld()->GetMapName();
	if (PlayerCharacter->AttributeSet)
	{
		CharData.SavedMaxHealth = PlayerCharacter->AttributeSet->GetMaxHealth();
	}
	CharData.InventoryJSON = PlayerCharacter->SerializeInventoryJSON();
	CharData.EquipmentJSON = PlayerCharacter->SerializeEquipmentJSON();
	CharData.QuestsJSON = TEXT("{}");
	const bool bSaved = DataSubsystem->SaveCharacterPreservingIdentity(PS->PlayerPlatform, PS->PlayerPlatformID, CharData);
	if (bSaved)
	{
		bCharacterDataSaved = true;
	}
	return bSaved;
}

void AOnsetPlayerController::PawnLeavingGame()
{
	// If autoplay is already active, the AI controller already possesses our pawn,
	// so GetPawn() is null here — adopt from the AI controller instead.
	APawn* LeavingPawn = GetPawn();
	if (LeavingPawn == nullptr && AutoCombatController)
	{
		LeavingPawn = AutoCombatController->GetPawn();
	}

	// Nothing to hand over — defer to the default behavior (destroys nothing here).
	if (!LeavingPawn)
	{
		Super::PawnLeavingGame();
		return;
	}

	AOnsetPlayerState* PS = GetPlayerState<AOnsetPlayerState>();
	const bool bShouldContinue = PS && PS->bContinueOnDisconnect && AutoCombatController;

	// Save now while PS + pawn are still valid (EndPlay runs later with no pawn).
	SaveCurrentCharacter(LeavingPawn);

	if (!bShouldContinue)
	{
		Super::PawnLeavingGame();
		return;
	}

	// Hand the pawn to the auto-combat controller so it keeps fighting while the
	// player is offline, instead of destroying it. Leave our Pawn reference intact:
	// the engine's Destroyed() → UnPossess() will then clear it (and the pawn's
	// controller reference) cleanly before the AI controller adopts it next tick.
	UE_LOG(LogTemp, Warning, TEXT("PawnLeavingGame: handing pawn %s to auto-combat controller"),
		*GetNameSafe(LeavingPawn));
	AutoCombatController->AdoptAbandonedPawn(LeavingPawn, PS->PlayerPlatform, PS->PlayerPlatformID, PS->SelectedCharacterSlot);
}

void AOnsetPlayerController::Client_SessionToken_Implementation(const FString& Token)
{
	CachedSessionToken = Token;
	UE_LOG(LogOnsetAuth, Log, TEXT("Client_SessionToken: received (%d chars), stored for reconnect"), Token.Len());
}

void AOnsetPlayerController::Client_SessionTokenFailed_Implementation(const FString& Reason)
{
	UE_LOG(LogOnsetAuth, Error, TEXT("Client_SessionTokenFailed: %s"), *Reason);
}

void AOnsetPlayerController::Client_TravelToGameServer_Implementation(const FString& ServerIP, const FString& ServerPort, const FString& Token)
{
	CachedSessionToken = Token;
	FString URL = FString::Printf(TEXT("%s:%s/Game/Maps/DemoLevel?Token=%s"),
		*ServerIP, *ServerPort, *Token);

	UE_LOG(LogOnsetAuth, Log, TEXT("Client_TravelToGameServer: traveling to %s:%s with token"), *ServerIP, *ServerPort);
	UE_LOG(LogOnsetAuth, Log, TEXT("Travel URL: %s"), *URL);

	if (UOnsetUISubsystem* UI = GetGameInstance()->GetSubsystem<UOnsetUISubsystem>())
	{
		UI->ShowLoadingScreen();
	}

	ClientTravel(URL, TRAVEL_Absolute);
}

void AOnsetPlayerController::ReconnectToGameServer()
{
	if (CachedSessionToken.IsEmpty())
	{
		UE_LOG(LogOnsetAuth, Warning, TEXT("ReconnectToGameServer: no cached session token"));
		return;
	}

	FString GameServerIP = TEXT("127.0.0.1");
	FString GameServerPort = TEXT("7777");
	GConfig->GetString(TEXT("Onset.Auth"), TEXT("GameServerIP"), GameServerIP, GEngineIni);
	GConfig->GetString(TEXT("Onset.Auth"), TEXT("GameServerPort"), GameServerPort, GEngineIni);

	FString URL = FString::Printf(TEXT("%s:%s/Game/Maps/DemoLevel?Token=%s"),
		*GameServerIP, *GameServerPort, *CachedSessionToken);

	UE_LOG(LogOnsetAuth, Log, TEXT("ReconnectToGameServer: traveling to %s:%s with token"), *GameServerIP, *GameServerPort);

	if (UOnsetUISubsystem* UI = GetGameInstance()->GetSubsystem<UOnsetUISubsystem>())
	{
		UI->ShowLoadingScreen();
	}

	ClientTravel(URL, TRAVEL_Absolute);
}