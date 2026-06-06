// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "SpawnerSlot.generated.h"

class AOnsetEnemy;

/** A single spawn position tracked by the spawner. Occupant is the currently-spawned enemy, or null. */
USTRUCT()
struct FSpawnerSlot
{
	GENERATED_BODY()

	/** World transform at which the enemy spawns. */
	UPROPERTY()
	FTransform SpawnTransform;

	/** The enemy currently occupying this slot, or null if empty. */
	UPROPERTY(Transient)
	TObjectPtr<AOnsetEnemy> Occupant = nullptr;
	
	UPROPERTY()
	FTimerHandle RespawnTimerHandle;
};                                                                                                          