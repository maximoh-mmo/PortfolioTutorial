// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "OnsetPlayerState.generated.h"

/** Per-player replicated state. Currently holds the PvP toggle flag. */
UCLASS()
class ONSET_API AOnsetPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	/** Whether PvP is enabled for this player. Replicated; triggers OnRep on clients. */
	UPROPERTY(ReplicatedUsing=OnRep_PvPEnabled)
	bool bIsPvPEnabled = false;

	/** Steam auth ticket (server-only, never replicated). */
	UPROPERTY()
	FString SteamAuthTicket;

	/** Platform identifier (e.g. "Steam" / "Xbox" / "PSN"). Replicated for client display. */
	UPROPERTY(Replicated)
	FString PlayerPlatform;

	/** Platform-specific user ID (e.g. SteamID64 as string). Replicated for client display. */
	UPROPERTY(Replicated)
	FString PlayerPlatformID;

	/** Character slot selected by this player (0-2, -1 = none). Server-only. */
	UPROPERTY()
	int32 SelectedCharacterSlot = -1;

	/** Zone entry point to spawn at after travel. Set by zone gate, consumed by ChoosePlayerStart. Server-only. */
	UPROPERTY()
	FString PendingEntryPoint;

	/** Called when bIsPvPEnabled changes on a client. Updates TargetingComponent validation. */
	UFUNCTION()
	void OnRep_PvPEnabled();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
};
