// Fill out your copyright notice in the Description page of Project Settings.


#include "Multiplayer/OnsetGameModeBase.h"

#include "Engine/Engine.h"
#include "Engine/PlayerStartPIE.h"
#include "EngineUtils.h"
#include "GAS/OnsetAttributeSet.h"
#include "Multiplayer/OnsetGameState.h"
#include "Player/OnsetPlayerCharacter.h"
#include "Player/OnsetPlayerController.h"
#include "Player/OnsetPlayerState.h"
#include "Subsystem/OnsetPlayerDataSubsystem.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogSteamAuth);

	AOnsetGameModeBase::AOnsetGameModeBase()
{
	DefaultPawnClass = AOnsetPlayerCharacter::StaticClass();
	PlayerControllerClass = AOnsetPlayerController::StaticClass();
	PlayerStateClass = AOnsetPlayerState::StaticClass();
	GameStateClass = AOnsetGameState::StaticClass();
	HUDClass = nullptr;
}

void AOnsetGameModeBase::StartPlay()
{
	Super::StartPlay();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("GameMode Started!"));
	}
}

void AOnsetGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!NewPlayer || !HasAuthority())
		return;

	AOnsetPlayerState* PS = NewPlayer->GetPlayerState<AOnsetPlayerState>();
	if (!PS)
		return;

	// Platform ID defaults based on the network transport
	FString Platform = TEXT("Steam");
	FString PlatformID = TEXT("");

	// Extract platform-specific user ID from the network connection
	FUniqueNetIdRepl UniqueId = PS->GetUniqueId();
	if (UniqueId.IsValid())
	{
		PlatformID = UniqueId->ToString();
	}

	PS->PlayerPlatform = Platform;
	PS->PlayerPlatformID = PlatformID;

	UE_LOG(LogSteamAuth, Log, TEXT("PostLogin: player %s — platform=%s, id=%s"),
		*NewPlayer->GetName(), *Platform, *PlatformID);

	// Load or create persistent account data
	UOnsetPlayerDataSubsystem* DataSubsystem = GetWorld()->GetSubsystem<UOnsetPlayerDataSubsystem>();
	if (!DataSubsystem)
	{
		UE_LOG(LogSteamAuth, Warning, TEXT("PostLogin: UOnsetPlayerDataSubsystem not available"));
		return;
	}

	FOnsetAccountData AccountData;
	if (!DataSubsystem->LoadAccount(Platform, PlatformID, AccountData))
	{
		// No account found — auto-create on first login
		if (DataSubsystem->CreateAccount(Platform, PlatformID))
		{
			UE_LOG(LogSteamAuth, Log, TEXT("PostLogin: auto-created account for %s/%s"), *Platform, *PlatformID);
			// Reload the freshly-created account
			DataSubsystem->LoadAccount(Platform, PlatformID, AccountData);
		}
		else
		{
			UE_LOG(LogSteamAuth, Error, TEXT("PostLogin: failed to create account for %s/%s"), *Platform, *PlatformID);
			return;
		}
	}

	// If player already selected a character, this is a zone-travel reconnect — skip sending account data
	AOnsetPlayerController* PC = Cast<AOnsetPlayerController>(NewPlayer);
	if (PC && PS->SelectedCharacterSlot < 0)
	{
		PC->Client_AccountData(AccountData);
	}
	else if (PC && PS->SelectedCharacterSlot >= 0)
	{
		UE_LOG(LogSteamAuth, Log, TEXT("PostLogin: player %s already has slot %d (zone travel) — skipping account UI"),
			*NewPlayer->GetName(), PS->SelectedCharacterSlot);
	}
}

void AOnsetGameModeBase::ValidateAuthTicket(APlayerController* NewPlayer, const FString& AuthTicket)
{
	if (!NewPlayer) return;

	if (AuthTicket.IsEmpty())
	{
		UE_LOG(LogSteamAuth, Error, TEXT("Steam auth failed — empty ticket from player, kicking."));
		NewPlayer->Destroy();
		return;
	}

	AOnsetPlayerController* PC = Cast<AOnsetPlayerController>(NewPlayer);
	if (PC)
	{
		PC->ClearAuthTimeout();
		PC->Client_ClearAuthTimeout();
	}

	AOnsetPlayerState* PS = NewPlayer->GetPlayerState<AOnsetPlayerState>();
	if (PS)
	{
		PS->SteamAuthTicket = AuthTicket;
	}

	UE_LOG(LogSteamAuth, Log, TEXT("Steam auth ticket accepted for player %s (%d chars)."),
		*NewPlayer->GetName(), AuthTicket.Len());
}

AActor* AOnsetGameModeBase::ChoosePlayerStart_Implementation(AController* Player)
{
	AOnsetPlayerState* PS = Player ? Player->GetPlayerState<AOnsetPlayerState>() : nullptr;

	// If player has a pending entry point, find a PlayerStart tagged with that name
	if (PS && !PS->PendingEntryPoint.IsEmpty())
	{
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			APlayerStart* Start = *It;
			if (Start && Start->Tags.Contains(FName(*PS->PendingEntryPoint)))
			{
				UE_LOG(LogSteamAuth, Log, TEXT("ChoosePlayerStart: matched entry point '%s' -> %s"),
					*PS->PendingEntryPoint, *Start->GetName());
				PS->PendingEntryPoint.Empty();
				return Start;
			}
		}
		UE_LOG(LogSteamAuth, Warning, TEXT("ChoosePlayerStart: entry point '%s' not found — falling back to default"),
			*PS->PendingEntryPoint);
		PS->PendingEntryPoint.Empty();
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

			DataSubsystem->SaveCharacter(PS->PlayerPlatform, PS->PlayerPlatformID, CharData);

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