// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "OnsetPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnsetPlayerSettingsChanged);

/** Per-player replicated state. Currently holds the PvP toggle flag. */
UCLASS()
class ONSET_API AOnsetPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	/** Whether PvP is enabled for this player. Replicated; triggers OnRep on clients. */
	UPROPERTY(ReplicatedUsing=OnRep_PvPEnabled)
	bool bIsPvPEnabled = false;

	/** Whether autoplay (AI possession of the pawn) is currently enabled. Replicated; drives the HUD toggle. */
	UPROPERTY(ReplicatedUsing=OnRep_AutoplayEnabled)
	bool bAutoplayEnabled = false;

	/** Whether the pawn keeps auto-combating after this player disconnects. Session-only, defaults to on. */
	UPROPERTY(ReplicatedUsing=OnRep_ContinueOnDisconnect)
	bool bContinueOnDisconnect = true;

	/** Seconds of input silence before autoplay re-engages; 0 = never auto-engage.
	*  Owner-replicated; authored via Server_SetIdleAutoCombatDelay (autoplay settings menu). */
	UPROPERTY(ReplicatedUsing=OnRep_IdleAutoCombatDelay)
	float IdleAutoCombatDelaySeconds = 5.0f;

	/** Server-side clamp-and-apply for IdleAutoCombatDelaySeconds (negative values clamped to 0). */
	void SetIdleAutoCombatDelaySeconds(float Seconds);

	
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

	/** Called when bAutoplayEnabled changes on a client. Refreshes the HUD autoplay toggle. */
	UFUNCTION()
	void OnRep_AutoplayEnabled();

	/** Called when bContinueOnDisconnect changes on a client. Refreshes the HUD toggle. */
	UFUNCTION()
	void OnRep_ContinueOnDisconnect();

	/** Called when IdleAutoCombatDelaySeconds changes on a client. Refreshes settings UI. */
	UFUNCTION()
	void OnRep_IdleAutoCombatDelay();
	
	/** Broadcast on the local client whenever autoplay/continue-on-disconnect state replicates. */
	UPROPERTY(BlueprintAssignable, Category = "Settings")
	FOnsetPlayerSettingsChanged OnPlayerSettingsChanged;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
};
