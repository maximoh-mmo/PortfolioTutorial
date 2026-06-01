// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OnsetPoolManager.generated.h"

class AOnsetAIController;
class AOnsetEnemy;
DECLARE_LOG_CATEGORY_EXTERN(LogPooling, Log, All);
/** Pre-allocates AOnsetEnemy actors for reuse, avoiding mid-game SpawnActor overhead. */
UCLASS()
class ONSET_API AOnsetPoolManager : public AActor
{
	GENERATED_BODY()

public:
	AOnsetPoolManager();

	/** Number of NPCs to pre-allocate on BeginPlay. */
	UPROPERTY(EditAnywhere, Category = "Pooling")
	int32 PoolSize = 10;

	/** Retrieve a deactivated enemy from the pool. Spawns a fallback if exhausted. */
	UFUNCTION(BlueprintCallable, Category = "Pooling")
	AOnsetEnemy* GetPooledEnemy();
	AOnsetAIController* GetPooledController();

	/** Return an enemy to the pool for later reuse. */
	UFUNCTION(BlueprintCallable, Category = "Pooling")
	void ReleasePooledEnemy(AOnsetEnemy* Enemy);
	void ReleasePooledController(AOnsetAIController* Controller);

	/** Pre-allocate all pool members. Safe to call multiple times. */
	UFUNCTION(BlueprintCallable, Category = "Pooling")
	void InitializePool();

protected:
	virtual void BeginPlay() override;

	/** All pool members. Hidden and deactivated when idle. */
	UPROPERTY()
	TArray<AOnsetEnemy*> ObjectPool;
	
	UPROPERTY()
	TArray<AOnsetAIController*> ControllerPool;
	
private:
	bool bPoolInitialized = false;

	/** Hide, disable, and reset a used enemy back into the pool. */
	void ReturnToPool(AOnsetEnemy* Enemy);
};
