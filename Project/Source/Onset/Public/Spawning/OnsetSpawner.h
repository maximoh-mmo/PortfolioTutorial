// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SpawnConfig.h"
#include "SpawnerSlot.h"
#include "GameFramework/Actor.h"
#include "OnsetSpawner.generated.h"

class ASpawnPoint;
class UGroupManagerComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogSpawner, Log, All);

/** Level actor that spawns groups of enemies at configurable spawn points or in a ring scatter. */
UCLASS(Blueprintable)
class ONSET_API AOnsetSpawner : public AActor
{
	GENERATED_BODY()

public:
	AOnsetSpawner();
	/** Configuration for spawning */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawning")
	FSpawnConfig Config;

	/** Optional explicit array of points where actors can be spawned;
	 *  falls back to ring scatter around the spawner location. */
	UPROPERTY(EditInstanceOnly, Category="Spawning")
	TArray<TObjectPtr<ASpawnPoint>> SpawnPoints;
	
	/** Group manager created automatically as a subobject. Tracks group membership. */
	UPROPERTY(VisibleAnywhere, Category = "Spawning")
	UGroupManagerComponent* GroupManager;

	/** Spawns one full group based on SpawnConfig. */
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void SpawnGroup();

	/** Destroys all currently spawned NPCs and returns them to pool. */
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void DestroyGroup();

	/** Kills and returns all NPCs in the group. */
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void DebugKillAll();

	/** Kills and returns the most recently spawned NPC. */
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void DebugKillLast();
	
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void OnNPCDeath(AOnsetEnemy* Enemy);
	void RespawnNPC(int32 SlotIndex);

#if WITH_EDITOR
	/** Spawns and attaches a new spawn point actor at this spawner's location. Editor-only. */
	ASpawnPoint* CreateSpawnPoint();

	/**
	 * Re-arranges auto-placed spawn points into an equidistant ring around this
	 * spawner at Config.SpawnRadius. Points the user has moved by hand are left
	 * untouched but still count toward the ring's slot count.
	 */
	void RelocateSpawnPoints();
		
	virtual void PostEditChangeProperty(
		struct FPropertyChangedEvent& PropertyChangedEvent
		) override;
	void SyncSpawnPoints();

	virtual void PostEditUndo() override;

	virtual void PostLoad() override;
#endif
	
#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient)
	TArray<TObjectPtr<ASpawnPoint>> EditorSpawnPointCache;
#endif
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Pre-compute spawn transforms from SpawnPoints or fallback ring scatter. */
	void InitSlots();

	/** Spawn one enemy into the given slot index. */
	AOnsetEnemy* SpawnEnemyAtSlot(int32 SlotIndex);

private:
	/** If true, SpawnGroup() is called automatically during BeginPlay. */
	UPROPERTY(EditAnywhere, Category="Spawning")
	bool bAutoSpawn = true;

	/** Internal slot array holding spawn transforms and occupant pointers. */
	UPROPERTY()
	TArray<FSpawnerSlot> Slots;
	
#if WITH_EDITOR
	void UpdateAllSpawnPointPreviews();

	/** World-space location for the point at the given index on an equidistant ring. */
	FVector GetIdealSpawnPointLocation(int32 Index, int32 Count) const;
	
	void RemoveInvalidSpawnPoints();
	
	void DestroySpawnPoint(ASpawnPoint* SpawnPoint);
#endif
	
};
