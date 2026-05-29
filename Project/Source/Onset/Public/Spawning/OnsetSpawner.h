// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SpawnConfig.h"
#include "SpawnerSlot.h"
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
	
	/** Pool manager for handling NPC pooling. */
	UPROPERTY(EditAnywhere, Category="Spawning")
	AOnsetPoolManager* PoolManager;
	
	UPROPERTY(VisibleAnywhere, Category = "Spawning")                                                               
	UGroupManagerComponent* GroupManager;     
		
	/** Spawns one full group based on SpawnConfig. */                                                          
	UFUNCTION(BlueprintCallable, Category = "Spawner")                                                          
	void SpawnGroup();                                                                                          
                                                                                                                     
	/** Destroys all currently spawned NPCs. */                                                                 
	UFUNCTION(BlueprintCallable, Category = "Spawner")                                                          
	void DestroyGroup();        
	
	/** Single NPC spawner for manual / temp testing. */                                                        
	UFUNCTION(BlueprintCallable, Category = "Spawner")                                                          
	void SpawnSingleNPC();
	
	UFUNCTION(BLueprintCallable, Category = "Spawner")
	void DebugKillAll();
	
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void DebugKillLast();
		
protected:                                                                                                      
	virtual void BeginPlay() override;  
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;                                                            
                                                               	
	void InitSlots();
	AOnsetEnemy* SpawnEnemyAtSlot(int32 SlotIndex);
	                                       	
private:       
	UPROPERTY(EditAnywhere, Category="Spawning")                                                                    
	bool bAutoSpawn = true;     
	
	UPROPERTY()
	TArray<FSpawnerSlot> Slots;        
};
