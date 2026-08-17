// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "SpawnConfig.generated.h"

class UAIProfile;
class UVisualProfile;
class UPerceptionProfile;

/** Configuration for a single spawner: which profile, group size, and placement. */
USTRUCT(BlueprintType)
struct FSpawnConfig
{
	GENERATED_BODY()

	/** The UAIProfile asset defining behaviors for spawned NPCs. */
	UPROPERTY(EditAnywhere, Category = "Spawning")
	TObjectPtr<UAIProfile> EnemyAIProfile;
	
	/** The UVisualProfile asset defining visuals for spawned NPCs. */
	UPROPERTY(EditAnywhere, Category = "Spawning")
	TObjectPtr<UVisualProfile> EnemyVisualProfile;
	
	/** The UPerceptionProfile asset defining perception for spawned NPCs. */
	UPROPERTY(EditAnywhere, Category = "Spawning")
	TObjectPtr<UPerceptionProfile> EnemyPerceptionProfile;

	/** Number of enemies per spawn group. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	int32 GroupSize = 5;

	/** Distance from spawner center when using fallback ring scatter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	float SpawnRadius = 300.0f;

	/** Delay in seconds before respawning a destroyed group member (future use). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	float RespawnDelay = 10.0f;

	/**
	 * DT_EnemyStats row for spawned enemies (empty row = default stats).
	 * RowType scopes the table picker to FOnsetEnemyStats tables; the row name
	 * dropdown is populated from the assigned table's rows.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner", meta = (RowType = "/Script/Onset.OnsetEnemyStats"))
	FDataTableRowHandle EnemyStats;

	/** Difficulty tier; stats scale by (1 + d)^Tier (d = 15%). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	int32 Tier = 0;

	/** Area tag stamped on spawned enemies; gates zone-scoped loot entries. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	FGameplayTag ZoneTag;
};
