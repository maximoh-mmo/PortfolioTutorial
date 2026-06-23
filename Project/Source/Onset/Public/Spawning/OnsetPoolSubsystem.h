// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OnsetPoolSubsystem.generated.h"

class AOnsetAIController;
class AOnsetEnemy;

DECLARE_LOG_CATEGORY_EXTERN(LogPooling, Log, All);
/** Pre-allocates AOnsetEnemy actors for reuse, avoiding mid-game SpawnActor overhead. */
UCLASS()
class ONSET_API UOnsetPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	
	/** Retrieve a deactivated enemy from the pool. Spawns a fallback if exhausted. */
	UFUNCTION(BlueprintCallable, Category = "Pooling")
	AOnsetEnemy* GetPooledEnemy();
	UFUNCTION(BlueprintCallable, Category = "Pooling")
	AOnsetAIController* GetPooledController();

	/** Return an enemy to the pool for later reuse. */
	UFUNCTION(BlueprintCallable, Category = "Pooling")
	void ReleasePooledEnemy(AOnsetEnemy* Enemy);
	UFUNCTION(BlueprintCallable, Category = "Pooling")
	void ReleasePooledController(AOnsetAIController* Controller);

	/** Pre-allocate all pool members. Safe to call multiple times. */
	UFUNCTION(BlueprintCallable, Category = "Pooling")
	void InitializePool();

	/** Number of NPCs to pre-allocate on BeginPlay. */
	UPROPERTY(EditAnywhere, Category = "Pooling")
	int32 PoolSize = 10;

	const TArray<TWeakObjectPtr<AOnsetEnemy>>& GetActiveEnemies() const { return ActiveEnemies; }  
	
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	
protected:

	/** All pool members. Hidden and deactivated when idle. */
	UPROPERTY()
	TArray<AOnsetEnemy*> ObjectPool;
	
	UPROPERTY()
	TArray<AOnsetAIController*> ControllerPool;
	
private:
	bool bPoolInitialized = false;

	/** Hide, disable, and reset a used enemy back into the pool. */
	void ReturnToPool(AOnsetEnemy* Enemy);
	
	UPROPERTY()
	TArray<TWeakObjectPtr<AOnsetEnemy>> ActiveEnemies;
};
