// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SpawnConfig.h"
#include "GameFramework/Actor.h"
#include "OnsetSpawner.generated.h"

class AOnsetPoolManager;
class UGroupManagerComponent;;

DECLARE_LOG_CATEGORY_EXTERN(LogSpawner, Log, All);

UCLASS(Blueprintable)
class ONSET_API AOnsetSpawner : public AActor
{
	GENERATED_BODY()

public:
	AOnsetSpawner();
	
	/** Configuration for spawning */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawning")
	FSpawnConfig Config;
	
	/** Optional explicit array of points where actors can be spawned
	 * falls back to random/scatter around spawner. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawning")
	TArray<AActor*> SpawnPoints;
	
	/** Whether to spawn on BeginPlay. */
	UPROPERTY(EditAnywhere, Category="Spawning")
	bool bAutoSpawn = true;
	
	/** Pool manager for handling NPC pooling. */
	UPROPERTY(EditAnywhere, Category="Spawning")
	AOnsetPoolManager* PoolManager;
	
	/** Spawns one full group based on SpawnConfig. */                                                          
	UFUNCTION(BlueprintCallable, Category = "Spawner")                                                          
	void SpawnGroup();                                                                                          
                                                                                                                     
	/** Destroys all currently spawned NPCs. */                                                                 
	UFUNCTION(BlueprintCallable, Category = "Spawner")                                                          
	void DestroyGroup();        
	
	/** Single NPC spawner for manual / temp testing. */                                                        
	UFUNCTION(BlueprintCallable, Category = "Spawner")                                                          
	void SpawnSingleNPC();
	
	UPROPERTY(VisibleAnywhere, Category = "Spawning")                                                               
	UGroupManagerComponent* GroupManager;     
	
	UFUNCTION(BLueprintCallable, Category = "Spawner")
	void DebugKillAll();
	
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void DebugKillLast();
protected:                                                                                                      
	virtual void BeginPlay() override;                                                                          
                                                                                                                     
	/** Returns the spawn transform for index i — either from SpawnPoints or scatter fallback. */               
	FTransform GetSpawnLocation(int32 Index) const;                                                             
                                                                                                                     
	/** Tracks currently alive spawned NPCs. */                                                                 
	UPROPERTY()                                                                                                 
	TArray<AOnsetEnemy*> SpawnedGroup;
};
