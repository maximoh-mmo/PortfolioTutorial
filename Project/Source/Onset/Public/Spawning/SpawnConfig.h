// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SpawnConfig.generated.h"
/**
 * 
 */

USTRUCT(BlueprintType)
struct FSpawnConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	TSubclassOf<class AOnsetEnemy> EnemyClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	int32 GroupSize = 5;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	float SpawnRadius = 300.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	float RespawnDelay = 10.0f;
};
