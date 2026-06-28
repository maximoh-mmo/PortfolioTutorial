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

	/** Called when bIsPvPEnabled changes on a client. Updates TargetingComponent validation. */
	UFUNCTION()
	void OnRep_PvPEnabled();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
};
