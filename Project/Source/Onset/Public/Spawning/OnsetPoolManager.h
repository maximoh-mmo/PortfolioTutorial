// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OnsetPoolManager.generated.h"

class AOnsetEnemy;
DECLARE_LOG_CATEGORY_EXTERN(LogPooling, Log, All);
UCLASS()
class ONSET_API AOnsetPoolManager : public AActor
{
	GENERATED_BODY()
	
public:                                                                                                         
	AOnsetPoolManager();
	
	UPROPERTY(EditAnywhere, Category = "Pooling")                                                               
	int32 PoolSize = 10;                                                                                        
                                                                                                                     
	UPROPERTY(EditAnywhere, Category = "Pooling")                                                               
	TSubclassOf<AOnsetEnemy> PoolClass;                                                                         
                                                                                                                     
	UFUNCTION(BlueprintCallable, Category = "Pooling")                                                          
	AOnsetEnemy* GetPooledEnemy();                                                                                      
                                                                                                                     
	UFUNCTION(BlueprintCallable, Category = "Pooling")                                                          
	void ReleasePooledEnemy(AOnsetEnemy* Enemy);
	
	UFUNCTION(BlueprintCallable, Category = "Pooling")
	void InitializePool();
                                                                                                                     
protected:
	virtual void BeginPlay() override;
	
	UPROPERTY()                                                                                                 
	TArray<AOnsetEnemy*> ObjectPool;

private:
	bool bPoolInitialized = false;	
	void ActivateEnemy(AOnsetEnemy* Enemy);
	void ReturnToPool(AOnsetEnemy* Enemy);     	
};
