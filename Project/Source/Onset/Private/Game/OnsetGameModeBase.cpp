// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/OnsetGameModeBase.h"

#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"
#include "Engine/GameInstance.h"
#include "GAS/OnsetAttributeSet.h"
#include "Game/OnsetGameState.h"
#include "Player/OnsetPlayerCharacter.h"
#include "Player/OnsetPlayerController.h"
#include "Player/OnsetPlayerState.h"
#include "Subsystem/OnsetPlayerDataSubsystem.h"
#include "Subsystem/OnsetAuthSubsystem.h"
#include "Subsystem/OnsetUISubsystem.h"
#include "UI/OnsetRootLayout.h"
#include "UI/OnsetScreenBase.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogSteamAuth);

	AOnsetGameModeBase::AOnsetGameModeBase()
{
	DefaultPawnClass = AOnsetPlayerCharacter::StaticClass();
	static ConstructorHelpers::FClassFinder<AOnsetPlayerCharacter> PlayerBP(TEXT("/Game/Characters/Player/PlayerCharacter"));
	if (PlayerBP.Class != nullptr)
	{
		DefaultPawnClass = PlayerBP.Class;
	}
	PlayerControllerClass = AOnsetPlayerController::StaticClass();
	static ConstructorHelpers::FClassFinder<APlayerController> ControllerBP(TEXT("/Game/Input/MyOnsetPlayerController.MyOnsetPlayerController_C"));
	if (ControllerBP.Class != nullptr)
	{
		PlayerControllerClass = ControllerBP.Class;
	}
	PlayerStateClass = AOnsetPlayerState::StaticClass();
	GameStateClass = AOnsetGameState::StaticClass();
	HUDClass = nullptr;

	static ConstructorHelpers::FClassFinder<UOnsetRootLayout> RootLayoutClassFinder(TEXT("/Game/UI/Core/WBP_RootLayout"));
	if (RootLayoutClassFinder.Succeeded())
	{
		RootLayoutClass = RootLayoutClassFinder.Class;
	}
	static ConstructorHelpers::FClassFinder<UOnsetScreenBase> MenuFinder(TEXT("/Game/UI/Screens/WBP_MainMenu"));
	if (MenuFinder.Succeeded())
	{
		MainMenuScreenClass = MenuFinder.Class;
	}
}

void AOnsetGameModeBase::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (!ErrorMessage.IsEmpty())
		return;

	UOnsetAuthSubsystem* Auth = GetWorld()->GetSubsystem<UOnsetAuthSubsystem>();
	if (Auth && Auth->GetAuthMode() == EOnsetAuthMode::Token)
	{
		FString Platform, PlatformID;
		FString TokenError = Auth->PreLoginTokenAuth(Options, Address, Platform, PlatformID);
		if (!TokenError.IsEmpty())
		{
			ErrorMessage = TokenError;
			UE_LOG(LogOnsetAuth, Warning, TEXT("PreLogin: rejected connection from %s — %s"), *Address, *TokenError);
		}
	}
	else if (Auth)
	{
		Auth->PreLoginDirect(Options, Address);
	}
}

void AOnsetGameModeBase::StartPlay()
{
	Super::StartPlay();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("GameMode Started!"));
	}
}

void AOnsetGameModeBase::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	AOnsetPlayerState* PS = NewPlayer ? NewPlayer->GetPlayerState<AOnsetPlayerState>() : nullptr;

	// Reconnecting or zone-travel — spawn pawn normally
	if (PS && PS->SelectedCharacterSlot >= 0)
	{
		Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	}
	// New player — no pawn until character is selected via Server_SelectCharacter
}

void AOnsetGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!NewPlayer || !HasAuthority())
		return;

	UOnsetAuthSubsystem* Auth = GetWorld()->GetSubsystem<UOnsetAuthSubsystem>();
	if (Auth)
	{
		Auth->HandlePostLogin(NewPlayer);
	}

	AOnsetPlayerState* PS = NewPlayer->GetPlayerState<AOnsetPlayerState>();
	AOnsetPlayerController* PC = Cast<AOnsetPlayerController>(NewPlayer);

	if (Auth && Auth->GetAuthMode() == EOnsetAuthMode::Token)
	{
		if (PC && PS && PS->SelectedCharacterSlot >= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("GameMode::PostLogin: RestartPlayer for slot %d"), PS->SelectedCharacterSlot);
			RestartPlayer(NewPlayer);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("GameMode::PostLogin: token mode, NOT restarting (slot=%d) — client will hang on loading screen"),
				PS ? PS->SelectedCharacterSlot : -1);
		}
	}
	else if (PC && PS && PS->SelectedCharacterSlot < 0)
	{
		PC->Client_ShowMainMenuUI(RootLayoutClass, MainMenuScreenClass);
	}
}

void AOnsetGameModeBase::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	UOnsetAuthSubsystem* Auth = GetWorld()->GetSubsystem<UOnsetAuthSubsystem>();
	if (Auth)
	{
		Auth->HandleLogout(Exiting);
	}
}



AActor* AOnsetGameModeBase::ChoosePlayerStart_Implementation(AController* Player)
{
	AOnsetPlayerState* PlayerState = Player ? Player->GetPlayerState<AOnsetPlayerState>() : nullptr;

	// If player has a pending entry point, find a PlayerStart tagged with that name
	if (PlayerState && !PlayerState->PendingEntryPoint.IsEmpty())
	{
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			APlayerStart* PlayerStart = *It;
			if (PlayerStart && PlayerStart->Tags.Contains(FName(*PlayerState->PendingEntryPoint)))
			{
				UE_LOG(LogSteamAuth, Log, TEXT("ChoosePlayerStart: matched entry point '%s' -> %s"),
					*PlayerState->PendingEntryPoint, *PlayerStart->GetName());
				PlayerState->PendingEntryPoint.Empty();
				return PlayerStart;
			}
		}
		UE_LOG(LogSteamAuth, Warning, TEXT("ChoosePlayerStart: entry point '%s' not found — falling back to default"),
			*PlayerState->PendingEntryPoint);
		PlayerState->PendingEntryPoint.Empty();
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

void AOnsetGameModeBase::TravelToZone(const FString& MapName, const FString& EntryPoint)
{
	if (MapName.IsEmpty()) return;

	// Save all connected players' state before traveling
	UOnsetPlayerDataSubsystem* DataSubsystem = GetWorld()->GetSubsystem<UOnsetPlayerDataSubsystem>();
	if (DataSubsystem)
	{
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PC = It->Get();
			if (!PC || !PC->GetPawn()) continue;

			AOnsetPlayerState* PS = PC->GetPlayerState<AOnsetPlayerState>();
			if (!PS || PS->SelectedCharacterSlot < 0) continue;

			AOnsetPlayerCharacter* PlayerChar = Cast<AOnsetPlayerCharacter>(PC->GetPawn());
			if (!PlayerChar) continue;

			FOnsetFullCharacterData CharData;
			CharData.SlotIndex = PS->SelectedCharacterSlot;
			CharData.CurrentZone = MapName;
			CharData.SavedPosition = PlayerChar->GetActorLocation();
			CharData.SavedRotationYaw = PlayerChar->GetActorRotation().Yaw;
			if (PlayerChar->AttributeSet)
				CharData.SavedMaxHealth = PlayerChar->AttributeSet->GetMaxHealth();
			CharData.InventoryJSON = TEXT("{}");
			CharData.EquipmentJSON = TEXT("{}");
			CharData.QuestsJSON = TEXT("{}");

			DataSubsystem->SaveCharacterPreservingIdentity(PS->PlayerPlatform, PS->PlayerPlatformID, CharData);

			// Set entry point for this player
			if (!EntryPoint.IsEmpty())
				PS->PendingEntryPoint = EntryPoint;
		}
	}

	UE_LOG(LogSteamAuth, Log, TEXT("TravelToZone: traveling to '%s' (entry=%s)"), *MapName, *EntryPoint);
	GetWorld()->ServerTravel(MapName);
}

void AOnsetGameModeBase::TravelZone(const FString& MapName, const FString& EntryPoint)
{
	TravelToZone(MapName, EntryPoint);
}