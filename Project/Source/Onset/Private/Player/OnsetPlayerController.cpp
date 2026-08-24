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
#include "MeshAttributes.h"
#include "NavigationSystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "TimerManager.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Player/InteractionComponent.h"
#include "Player/OnsetMovementValidationComponent.h"
#include "Core/OnsetBaseCharacter.h"
#include "Engine/GameInstance.h"
#include "Player/OnsetPlayerCharacter.h"
#include "Player/OnsetPlayerState.h"
#include "Quest/UOnsetQuestComponent.h"
#include "Subsystem/OnsetPlayerDataSubsystem.h"
#include "GAS/OnsetAttributeSet.h"
#include "Player/OnsetCheatManager.h"
#include "Player/OnsetPlayerAIController.h"
#include "UI/GamepadCursorWidget.h"
#include "UI/HUDWidget.h"
#include "Subsystem/OnsetUISubsystem.h"
#include "UI/OnsetRootLayout.h"
#include "UI/OnsetScreenBase.h"
#include "UI/OnsetActivatableWidgetStack.h"
#include "UI/InventoryScreen.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Engine/DataTable.h"
#include "Data/OnsetClassInfoTypes.h"
#include "Data/OnsetItemLibrary.h"
#include "Inventory/UOnsetInventoryComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "NavFilters/NavigationQueryFilter.h"

DEFINE_LOG_CATEGORY(LogGamepad);
DEFINE_LOG_CATEGORY(LogPlayerNavigation);
DECLARE_CYCLE_STAT(TEXT("MoveTo"), STAT_MoveTo, STATGROUP_Navigation);

AOnsetPlayerController::AOnsetPlayerController()
{
	bSetControlRotationFromPawnOrientation = true;
	CursorManager = CreateDefaultSubobject<UCursorManager>(TEXT("CursorManager"));
	PathFollowingComponent = CreateDefaultSubobject<UPathFollowingComponent>(TEXT("PathFollowingComponent"));
	if (PathFollowingComponent)
	{
		PathFollowingComponent->OnRequestFinished.AddUObject(this, &AOnsetPlayerController::OnMoveCompleted);
	}
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
		EnhancedInputComponent->BindAction(IA_Inventory, ETriggerEvent::Started, this, &AOnsetPlayerController::OnInventoryToggleTriggered);
		
	}
}

void AOnsetPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (bIsFollowingPath && IsLocalController())
	{
		APawn* ControlledPawn = GetPawn();
		if (!ControlledPawn)
		{
			ControlledPawn = CachedPlayerPawn;
		}

		if (!ControlledPawn)
		{
			StopMovement();
			return;
		}

		// If following an actor, check if reached or invalid
		if (FollowTargetActor.IsValid())
		{
			const AOnsetBaseCharacter* TargetChar = Cast<AOnsetBaseCharacter>(FollowTargetActor.Get());
			if (TargetChar && !TargetChar->IsAlive())
			{
				StopMovement();
				return;
			}

			const float DistToActor = FVector::Dist2D(ControlledPawn->GetActorLocation(), FollowTargetActor->GetActorLocation());
			if (DistToActor <= PathAcceptanceRadius)
			{
				StopMovement();
				return;
			}
		}

		// Advance past reached waypoints
		while (CurrentPathIndex < ActivePathPoints.Num())
		{
			const FVector CurrentTarget = ActivePathPoints[CurrentPathIndex];
			const FVector PawnLocation = ControlledPawn->GetActorLocation();
			const float DistToWaypoint = FVector::Dist2D(PawnLocation, CurrentTarget);
			const float Acceptance = (CurrentPathIndex == ActivePathPoints.Num() - 1) ? PathAcceptanceRadius : FMath::Max(PathAcceptanceRadius, 80.0f);

			if (DistToWaypoint <= Acceptance)
			{
				CurrentPathIndex++;
			}
			else
			{
				break;
			}
		}

		if (CurrentPathIndex < ActivePathPoints.Num())
		{
			const FVector TargetPoint = ActivePathPoints[CurrentPathIndex];
			const FVector PawnLocation = ControlledPawn->GetActorLocation();
			FVector MoveDirection = TargetPoint - PawnLocation;
			MoveDirection.Z = 0.0f;
			if (!MoveDirection.IsNearlyZero())
			{
				MoveDirection.Normalize();
				ControlledPawn->AddMovementInput(MoveDirection, 1.0f);
			}
		}
		else
		{
			StopMovement();
		}
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
	if (PlayerChar->QuestComponent)
	{
		PlayerChar->QuestComponent->DeserializeQuestsJSON(CharData.QuestsJSON);
	}
	if (PlayerChar->AttributeSet)
	{
		PlayerChar->AttributeSet->SetHealth(PlayerChar->AttributeSet->GetMaxHealth());
	}

	// Apply persisted level/XP (combat-formulas §12) onto the freshly spawned pawn.
	PlayerChar->ApplyCharacterProgression(CharData.Level, CharData.Experience, CharData.UnspentStatPoints, CharData.PrestigeLevel);
	PlayerChar->SetPersistIdentity(PS->PlayerPlatform, PS->PlayerPlatformID, PS->SelectedCharacterSlot);

	PlayerChar->GrantDefaultAbilities();

	// Saved position was just restored - trust it (validator snap).
	if (PlayerChar->MovementValidator)
	{
		PlayerChar->MovementValidator->SnapToCurrentPosition();
	}

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

		// The loading screen tears the RootLayout down before travel (CleanupUI);
		// re-create it here so in-game screens (inventory) have a Game layer stack.
		// The HUD is added at ZOrder 1, so rebuild the layout above it (ZOrder 2).
		if (!UI->GetRootLayout() && GameRootLayoutClass)
		{
			UI->InitializeRootLayout(GameRootLayoutClass, 2);
		}
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
	if (!Self)
	{
		Self = Cast<AOnsetBaseCharacter>(CachedPlayerPawn);
	}

	if (!Self || !Self->AbilitySystemComponent || !Self->TargetingComponent || !BasicAttackAbility)
	{
		StopAutoAttack();
		return;
	}

	AActor* Target = Self->TargetingComponent->GetTarget();
	if (!Target || !Self->TargetingComponent->IsActorTargetValid(Target))
	{
		Self->TargetingComponent->ClearTarget();
		StopAutoAttack();
		return;
	}

	const float Dist2D = FVector::Dist2D(Self->GetActorLocation(), Target->GetActorLocation());
	const float MaxCombatRange = 300.0f;

	// If the target moved out of range and we are not already pathing, move towards it
	if (Dist2D > MaxCombatRange && !bIsFollowingPath && IsLocalController())
	{
		MoveToActor(Target, 200.0f);
	}

	Self->AbilitySystemComponent->TryActivateAbilityByClass(BasicAttackAbility);
}

void AOnsetPlayerController::OnMove(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	if (MovementVector.IsNearlyZero()) return;
	Server_DisableAutoCombat();
	ResetIdleTimer();
	StopMovement();
	StopAutoAttack();
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
	// Any gameplay-intent input interrupts autoplay and executes under player authority.
	Server_DisableAutoCombat();
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
	ProcessPrimaryInteraction(HitActor, HitLocation);
}

void AOnsetPlayerController::ProcessPrimaryInteraction(AActor* HitActor, FVector HitLocation)
{
	if (!InteractionComponent) return;
	if (HitActor && !IsValid(HitActor))
	{
		HitActor = nullptr;
	}
	// Process targeting.
	FMoveTarget MoveTarget = InteractionComponent->ProcessPrimaryInteraction(HitActor, HitLocation);
	if (MoveTarget.Actor.IsValid())
	{
		MoveToActor(MoveTarget.Actor.Get(), 200.0f);
	}
	else if (!MoveTarget.Position.IsZero())
	{
		MoveToLocation(MoveTarget.Position);
	}
}

void AOnsetPlayerController::Client_ShowLootOverlay_Implementation(const TArray<FOnsetInventoryEntry>& LootedItems)
{
	if (HUDWidget && LootedItems.Num() > 0)
	{
		HUDWidget->ShowLoot(LootedItems);
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

	const FName ItemRow = FName(*RowName);
	for (uint8 Index = 0; Index <= static_cast<uint8>(EOnsetItemCategory::Scroll); ++Index)
	{
		const EOnsetItemCategory Category = static_cast<EOnsetItemCategory>(Index);
		if (UOnsetItemLibrary::GetItemDefinition(Category, ItemRow))
		{
			PlayerCharacter->InventoryComponent->AddItem(Category, ItemRow, 1);
			UE_LOG(LogTemp, Log, TEXT("OnsetGrantItem: granted '%s' (%s)"),
				*RowName, *StaticEnum<EOnsetItemCategory>()->GetNameStringByValue(static_cast<int64>(Category)));
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("OnsetGrantItem: no item row named '%s' in any category table"), *RowName);
}

void AOnsetPlayerController::OnsetAcceptQuest(const FString& QuestRowName)
{
	if (QuestRowName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("OnsetAcceptQuest: empty quest row name"));
		return;
	}
	Server_AcceptQuest(QuestRowName);
}

void AOnsetPlayerController::Server_AcceptQuest_Implementation(const FString& QuestRowName)
{
	AOnsetPlayerCharacter* PlayerCharacter = Cast<AOnsetPlayerCharacter>(GetPawn());
	if (!PlayerCharacter || !PlayerCharacter->QuestComponent)
	{
		return;
	}

	const FName QuestRow = FName(*QuestRowName);
	PlayerCharacter->QuestComponent->AcceptQuest(QuestRow);
	UE_LOG(LogTemp, Log, TEXT("OnsetAcceptQuest: accepted '%s'"), *QuestRowName);
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

void AOnsetPlayerController::OnInventoryToggleTriggered(const FInputActionValue& Value)
{
	UOnsetUISubsystem* UI = GetGameInstance()->GetSubsystem<UOnsetUISubsystem>();
	if (!UI || !InventoryScreenClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("OnInventoryToggle: UI=%d ScreenClass=%d"), !!UI, !!InventoryScreenClass);
		return;
	}

	// If the inventory screen is already the active widget on the Game layer, pop it;
	// otherwise push it. Local-only toggle - no server RPC (UI is purely client-side).
	UOnsetActivatableWidgetStack* GameStack = UI->GetRootLayout()
		? UI->GetRootLayout()->GetStackForLayer(EOnsetUILayer::Game)
		: nullptr;
	if (GameStack && GameStack->GetActiveWidget() && GameStack->GetActiveWidget()->IsA<UInventoryScreen>())
	{
		UI->PopScreen(EOnsetUILayer::Game);
	}
	else
	{
		UI->PushScreen(EOnsetUILayer::Game, InventoryScreenClass);
	}
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
void AOnsetPlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);
	if (InPawn)
	{
		CachedPlayerPawn = InPawn;
		TargetingComponent = InPawn->FindComponentByClass<UTargetingComponent>();
	}
	if (PathFollowingComponent)
	{
		PathFollowingComponent->Initialize();
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
	const float Delay = GetEffectiveIdleDelay();
	bIdleTimerInitialized = true;

	if (!HasAuthority())
	{
		Server_ResetIdleTimer();
		return;
	}

	GetWorldTimerManager().ClearTimer(IdleAutoCombatTimerHandle);
	if (Delay > 0.0f)
	{	
		GetWorldTimerManager().SetTimer(IdleAutoCombatTimerHandle, this,
			&AOnsetPlayerController::EnableAutoCombat, Delay, false);
	}
}

void AOnsetPlayerController::Server_ResetIdleTimer_Implementation()
{
	ResetIdleTimer();
}

float AOnsetPlayerController::GetEffectiveIdleDelay() const
{
	if (const AOnsetPlayerState* PS = GetPlayerState<AOnsetPlayerState>())
	{
		return PS->IdleAutoCombatDelaySeconds;
	}
	return 0.0f;
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

			// Apply persisted level/XP (combat-formulas §12) so the pawn matches the saved character.
			PlayerCharacter->ApplyCharacterProgression(CharData.Level, CharData.Experience, CharData.UnspentStatPoints, CharData.PrestigeLevel);
			PlayerCharacter->SetPersistIdentity(PS->PlayerPlatform, PS->PlayerPlatformID, SlotIndex);

			PlayerCharacter->GrantDefaultAbilities();

			// Saved position was just restored - trust it (validator snap).
			if (PlayerCharacter->MovementValidator)
			{
				PlayerCharacter->MovementValidator->SnapToCurrentPosition();
			}
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
	CharData.QuestsJSON = PlayerCharacter->QuestComponent ? PlayerCharacter->QuestComponent->SerializeQuestsJSON() : TEXT("{}");

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
	CharData.QuestsJSON = PlayerCharacter->QuestComponent ? PlayerCharacter->QuestComponent->SerializeQuestsJSON() : TEXT("{}");
	const bool bSaved = DataSubsystem->SaveCharacterPreservingIdentity(PS->PlayerPlatform, PS->PlayerPlatformID, CharData);
	if (bSaved)
	{
		bCharacterDataSaved = true;
	}
	return bSaved;
}

void AOnsetPlayerController::Server_SetIdleAutoCombatDelay_Implementation(float Seconds)
{
	if (AOnsetPlayerState* PS = GetPlayerState<AOnsetPlayerState>())
	{
		PS->SetIdleAutoCombatDelaySeconds(Seconds);
	}
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

// Recreate AI Character movement from AIController

EPathFollowingRequestResult::Type AOnsetPlayerController::MoveToActor(AActor* Goal, float AcceptanceRadius,
	bool bStopOnOverlap, bool bUsePathfinding, bool bCanStrafe, TSubclassOf<UNavigationQueryFilter> FilterClass,
	bool bAllowPartialPath)
{
	if (bIsFollowingPath)
	{
		StopMovement();
	}

	FAIMoveRequest MoveReq(Goal);
	MoveReq.SetUsePathfinding(bUsePathfinding);
	MoveReq.SetAllowPartialPath(bAllowPartialPath);
	MoveReq.SetNavigationFilter(*FilterClass ? FilterClass : DefaultNavigationFilterClass);
	MoveReq.SetAcceptanceRadius(AcceptanceRadius);
	MoveReq.SetReachTestIncludesAgentRadius(bStopOnOverlap);
	MoveReq.SetCanStrafe(bCanStrafe);

	return MoveTo(MoveReq);
}

EPathFollowingRequestResult::Type AOnsetPlayerController::MoveToLocation(const FVector& Dest, float AcceptanceRadius,
	bool bStopOnOverlap, bool bUsePathfinding, bool bProjectDestinationToNavigation, bool bCanStrafe,
	TSubclassOf<UNavigationQueryFilter> FilterClass, bool bAllowPartialPath)
{
	if (bIsFollowingPath)
	{
		StopMovement();
	}

	FAIMoveRequest MoveReq(Dest);
	MoveReq.SetUsePathfinding(bUsePathfinding);
	MoveReq.SetAllowPartialPath(bAllowPartialPath);
	MoveReq.SetProjectGoalLocation(bProjectDestinationToNavigation);
	MoveReq.SetNavigationFilter(*FilterClass ? FilterClass : DefaultNavigationFilterClass);
	MoveReq.SetAcceptanceRadius(AcceptanceRadius);
	MoveReq.SetReachTestIncludesAgentRadius(bStopOnOverlap);
	MoveReq.SetCanStrafe(bCanStrafe);

	return MoveTo(MoveReq);
}

FPathFollowingRequestResult AOnsetPlayerController::MoveTo(const FAIMoveRequest& MoveRequest,
	FNavPathSharedPtr* OutPath)
{
	SCOPE_CYCLE_COUNTER(STAT_MoveTo);
	UE_LOG(LogPlayerNavigation, Log, TEXT("MoveTo: %s"), *MoveRequest.ToString());

	FPathFollowingRequestResult ResultData;
	ResultData.Code = EPathFollowingRequestResult::Failed;

	if (MoveRequest.IsValid() == false)
	{
		UE_LOG(LogPlayerNavigation, Error, TEXT("MoveTo request failed due MoveRequest not being valid. Most probably desired Goal Actor no longer exists. MoveRequest: '%s'"), *MoveRequest.ToString());
		return ResultData;
	}

	bool bCanRequestMove = true;
	bool bAlreadyAtGoal = false;
	const float EffectiveRadius = MoveRequest.GetAcceptanceRadius() > 0.0f ? MoveRequest.GetAcceptanceRadius() : 50.0f;

	if (!MoveRequest.IsMoveToActorRequest())
	{
		if (MoveRequest.GetGoalLocation().ContainsNaN() || FAISystem::IsValidLocation(MoveRequest.GetGoalLocation()) == false)
		{
			UE_LOG(LogPlayerNavigation, Error, TEXT("OnsetPlayerController::MoveTo: Destination is not valid! Goal(%s)"), TEXT_AI_LOCATION(MoveRequest.GetGoalLocation()));
			bCanRequestMove = false;
		}

		// fail if projection to navigation is required but it failed
		if (bCanRequestMove && MoveRequest.IsProjectingGoal())
		{
			UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
			const FNavAgentProperties& AgentProps = GetNavAgentPropertiesRef();
			FNavLocation ProjectedLocation;

			if (NavSys && !NavSys->ProjectPointToNavigation(MoveRequest.GetGoalLocation(), ProjectedLocation, INVALID_NAVEXTENT, &AgentProps))
			{
				if (MoveRequest.IsUsingPathfinding())
				{
					UE_LOG(LogPlayerNavigation, Error, TEXT("OnsetPlayerController::MoveTo failed to project destination location to navmesh. Location: %s"), *MoveRequest.GetGoalLocation().ToString());
				}
				else
				{
					UE_LOG(LogPlayerNavigation, Error, TEXT("OnsetPlayerController::MoveTo failed to project destination location to navmesh, path finding is disabled perhaps disable goal projection ?"));
				}

				bCanRequestMove = false;
			}

			MoveRequest.UpdateGoalLocation(ProjectedLocation.Location);
		}

		if (GetPawn())
		{
			bAlreadyAtGoal = bCanRequestMove && (FVector::Dist2D(GetPawn()->GetActorLocation(), MoveRequest.GetGoalLocation()) <= EffectiveRadius);
		}
	}
	else 
	{
		if (GetPawn() && MoveRequest.GetGoalActor())
		{
			bAlreadyAtGoal = bCanRequestMove && (FVector::Dist2D(GetPawn()->GetActorLocation(), MoveRequest.GetGoalActor()->GetActorLocation()) <= EffectiveRadius);
		}
	}

	if (bAlreadyAtGoal)
	{
		UE_LOG(LogPlayerNavigation, Log, TEXT("MoveTo: already at goal!"));
		ResultData.MoveId = FAIRequestID(1);
		ResultData.Code = EPathFollowingRequestResult::AlreadyAtGoal;
	}
	else if (bCanRequestMove)
	{
		FPathFindingQuery PFQuery;

		bool bShouldMergePaths = false;
		FVector StartLocation = GetNavAgentLocation();
		if (MoveRequest.ShouldStartFromPreviousPath())
		{
			if (ActivePathPoints.Num() > 0)
			{
				StartLocation = ActivePathPoints.Last();
				bShouldMergePaths = true;
			}
		}

		const bool bValidQuery = BuildPathfindingQuery(MoveRequest, StartLocation, PFQuery);
		if (bValidQuery)
		{
			FNavPathSharedPtr Path;
			FindPathForMoveRequest(MoveRequest, PFQuery, Path);

			if (Path.IsValid() && Path->GetPathPoints().Num() > 0)
			{
				if (!bShouldMergePaths)
				{
					ActivePathPoints.Empty();
				}

				const TArray<FNavPathPoint>& PathPoints = Path->GetPathPoints();
				const int32 StartIdx = (bShouldMergePaths || ActivePathPoints.Num() > 0) ? 1 : 0;
				for (int32 i = StartIdx; i < PathPoints.Num(); ++i)
				{
					ActivePathPoints.Add(PathPoints[i].Location);
				}

				if (!bShouldMergePaths)
				{
					CurrentPathIndex = (ActivePathPoints.Num() > 1) ? 1 : 0;
				}

				PathAcceptanceRadius = EffectiveRadius;
				FollowTargetActor = MoveRequest.IsMoveToActorRequest() ? MoveRequest.GetGoalActor() : nullptr;
				bIsFollowingPath = true;

				static uint32 NextMoveID = 1;
				ResultData.MoveId = FAIRequestID(NextMoveID++);
				ResultData.Code = EPathFollowingRequestResult::RequestSuccessful;

				if (OutPath)
				{
					*OutPath = Path;
				}
			}
		}
	}

	if (ResultData.Code == EPathFollowingRequestResult::Failed)
	{
		StopMovement();
	}

	return ResultData;
}

FAIRequestID AOnsetPlayerController::RequestMove(const FAIMoveRequest& MoveRequest, FNavPathSharedPtr Path)
{
	uint32 RequestID = FAIRequestID::InvalidRequest;
	if (PathFollowingComponent)
	{
		RequestID = PathFollowingComponent->RequestMove(MoveRequest, Path);
	}

	return RequestID;
}

void AOnsetPlayerController::FindPathForMoveRequest(const FAIMoveRequest& MoveRequest, FPathFindingQuery& Query,
	FNavPathSharedPtr& OutPath) const
{
	SCOPE_CYCLE_COUNTER(STAT_AI_Overall);

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys)
	{
		FPathFindingResult PathResult = NavSys->FindPathSync(Query);
		if (PathResult.Result != ENavigationQueryResult::Error)
		{
			if (PathResult.IsSuccessful() && PathResult.Path.IsValid())
			{
				if (MoveRequest.IsMoveToActorRequest())
				{
					PathResult.Path->SetGoalActorObservation(*MoveRequest.GetGoalActor(), 100.0f);
				}

				PathResult.Path->EnableRecalculationOnInvalidation(true);
				OutPath = PathResult.Path;
			}
		}
		else
		{
			UE_LOG(LogPlayerNavigation, Error, TEXT("Trying to find path to %s resulted in Error")
				, MoveRequest.IsMoveToActorRequest() ? *GetNameSafe(MoveRequest.GetGoalActor()) : *MoveRequest.GetGoalLocation().ToString());
			UE_LOG(LogPlayerNavigation, Error, TEXT("Failed move to %s"), GetPawn() ? *(GetPawn()->GetActorLocation()).ToString() : *FAISystem::InvalidLocation.ToString());
		}
	}
}

bool AOnsetPlayerController::BuildPathfindingQuery(const FAIMoveRequest& MoveRequest, FPathFindingQuery& OutQuery) const
{
	return BuildPathfindingQuery(MoveRequest, GetNavAgentLocation(), OutQuery);
}

bool AOnsetPlayerController::BuildPathfindingQuery(const FAIMoveRequest& MoveRequest, const FVector& StartLocation,
	FPathFindingQuery& OutQuery) const
{
	bool bResult = false;

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	const ANavigationData* NavData = (NavSys == nullptr) ? nullptr :
		MoveRequest.IsUsingPathfinding() ? NavSys->GetNavDataForProps(GetNavAgentPropertiesRef(), GetNavAgentLocation()) :
		NavSys->GetAbstractNavData();

	if (NavData)
	{
		FVector GoalLocation = MoveRequest.GetGoalLocation();
		if (MoveRequest.IsMoveToActorRequest())
		{
			const INavAgentInterface* NavGoal = Cast<const INavAgentInterface>(MoveRequest.GetGoalActor());
			if (NavGoal)
			{
				const FVector Offset = NavGoal->GetMoveGoalOffset(this);
				GoalLocation = FQuatRotationTranslationMatrix(MoveRequest.GetGoalActor()->GetActorQuat(), NavGoal->GetNavAgentLocation()).TransformPosition(Offset);
			}
			else
			{
				GoalLocation = MoveRequest.GetGoalActor()->GetActorLocation();
			}
		}

		FSharedConstNavQueryFilter NavFilter = UNavigationQueryFilter::GetQueryFilter(*NavData, this, MoveRequest.GetNavigationFilter());
		OutQuery = FPathFindingQuery(*this, *NavData, StartLocation, GoalLocation, NavFilter);
		OutQuery.SetAllowPartialPaths(MoveRequest.IsUsingPartialPaths());
		OutQuery.SetRequireNavigableEndLocation(MoveRequest.IsNavigableEndLocationRequired());
		if (MoveRequest.IsApplyingCostLimitFromHeuristic())
		{
			const float HeuristicScale = NavFilter->GetHeuristicScale();
			OutQuery.CostLimit = FPathFindingQuery::ComputeCostLimitFromHeuristic(OutQuery.StartLocation, OutQuery.EndLocation, HeuristicScale, MoveRequest.GetCostLimitFactor(), MoveRequest.GetMinimumCostLimit()); 
		}

		if (PathFollowingComponent)
		{
			PathFollowingComponent->OnPathfindingQuery(OutQuery);
		}

		bResult = true;
	}
	else
	{
		if (NavSys == nullptr)
		{
			UE_LOG(LogPlayerNavigation, Warning, TEXT("Unable OnsetPlayerController::BuildPathfindingQuery due to no NavigationSystem present. Note that even pathfinding-less movement requires presence of NavigationSystem."));
		}
		else 
		{
			UE_LOG(LogPlayerNavigation, Warning, TEXT("Unable to find NavigationData instance while calling OnsetPlayerController::BuildPathfindingQuery"));
		}
	}

	return bResult;
}

void AOnsetPlayerController::StopMovement()
{
	bIsFollowingPath = false;
	ActivePathPoints.Empty();
	CurrentPathIndex = 0;
	FollowTargetActor.Reset();

	if (InteractionComponent)
	{
		InteractionComponent->ClearPendingLoot();
	}

	if (PathFollowingComponent)
	{
		PathFollowingComponent->AbortMove(*this, FPathFollowingResultFlags::MovementStop);
	}
}

void AOnsetPlayerController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	ReceiveMoveCompleted.Broadcast(RequestID, Result.Code);
}

EPathFollowingStatus::Type AOnsetPlayerController::GetMoveStatus() const
{
	if (bIsFollowingPath)
	{
		return EPathFollowingStatus::Moving;
	}
	return (PathFollowingComponent) ? PathFollowingComponent->GetStatus() : EPathFollowingStatus::Idle;
}

bool AOnsetPlayerController::HasPartialPath() const
{
	return (PathFollowingComponent != NULL) && (PathFollowingComponent->HasPartialPath());
}

void AOnsetPlayerController::MergePaths(const FNavPathSharedPtr& InitialPath, FNavPathSharedPtr& InOutMergedPath) const
{
	if (!InitialPath.IsValid() || !InitialPath->IsValid())
	{
		UE_LOG(LogPlayerNavigation, Error, TEXT("%hs: InitialPath is Invalid"), __FUNCTION__);
		return;
	}

	if (!InOutMergedPath.IsValid() || !InOutMergedPath->IsValid())
	{
		UE_LOG(LogPlayerNavigation, Error, TEXT("%hs: InOutMergedPath is Invalid"), __FUNCTION__);
		return;
	}

	const TArray<FNavPathPoint>& InitialPathPoints = InitialPath->GetPathPoints();
	TArray<FNavPathPoint>& InOutPathPoints = InOutMergedPath->GetPathPoints();

	if (!InitialPathPoints.Last().Location.Equals(InOutPathPoints[0].Location))
	{
		UE_LOG(LogPlayerNavigation, Error, TEXT("%hs: last %s and first %s points don't match."), __FUNCTION__, *InitialPathPoints.Last().Location.ToString(), *InOutPathPoints[0].Location.ToString());
		return;
	}

	// We don't want to keep path points that have already been traversed, so only merge the points starting from "CurrentPathIndex".
	const int32 StartingPointIndex = PathFollowingComponent ? PathFollowingComponent->GetCurrentPathIndex() : 0;
	if (StartingPointIndex < InitialPathPoints.Num())
	{
		InOutPathPoints.Insert(&InitialPathPoints[StartingPointIndex], InitialPathPoints.Num() - StartingPointIndex - 1, 0);
	}
}

bool AOnsetPlayerController::IsFollowingAPath() const
{
	return bIsFollowingPath || ((PathFollowingComponent != nullptr) && (PathFollowingComponent->GetStatus() != EPathFollowingStatus::Idle));
}

IPathFollowingAgentInterface* AOnsetPlayerController::GetPathFollowingAgent() const
{
	return PathFollowingComponent;
}
