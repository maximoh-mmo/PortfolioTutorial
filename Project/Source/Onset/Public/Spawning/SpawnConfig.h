// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "SpawnConfig.generated.h"

class UAIProfile;

/** Configuration for a single spawner: which profile, group size, and placement. */
USTRUCT(BlueprintType)
struct FSpawnConfig
{
	GENERATED_BODY()

	/** The UAIProfile asset defining visuals and behaviour for spawned NPCs. */
	UPROPERTY(EditAnywhere, Category = "Spawning")
	TObjectPtr<UAIProfile> EnemyProfile;

	/** Number of enemies per spawn group. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	int32 GroupSize = 5;

	/** Distance from spawner center when using fallback ring scatter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	float SpawnRadius = 300.0f;

	/** Delay in seconds before respawning a destroyed group member (future use). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	float RespawnDelay = 10.0f;
};
