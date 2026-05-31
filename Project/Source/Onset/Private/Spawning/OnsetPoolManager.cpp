// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawning/OnsetPoolManager.h"

#include "AI/OnsetAIController.h"
#include "Components/StateTreeComponent.h"
#include "Enemy/GroupComponent.h"
#include "Enemy/OnsetEnemy.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogPooling);
// Sets default values
AOnsetPoolManager::AOnsetPoolManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AOnsetPoolManager::ActivateEnemy(AOnsetEnemy* Enemy)
{
	Enemy->SetActorHiddenInGame(false);                                                                   
	Enemy->SetActorTickEnabled(true);
	Enemy->SetActorEnableCollision(true);
}
	
AOnsetEnemy* AOnsetPoolManager::GetPooledEnemy()
{
	if (!bPoolInitialized) InitializePool();
	for (AOnsetEnemy* Enemy : ObjectPool)
	{
		if (Enemy && Enemy->IsHidden())
		{                                                                        
			ActivateEnemy(Enemy);                                                                     
			return Enemy;
		}
	}
	// Pool exhausted — fallback SpawnActor (hardcoded to base AOnsetEnemy)                                          
	UE_LOG(LogPooling, Warning, TEXT("OnsetPoolManager: Pool exhausted — spawning new NPC as fallback."));
	FActorSpawnParameters Params;                                                                           
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AOnsetEnemy* Enemy = GetWorld()->SpawnActor<AOnsetEnemy>(AOnsetEnemy::StaticClass(), FTransform::Identity, Params))
	{
		ObjectPool.Add(Enemy);          
		return Enemy;                                                                                         
	}
	return nullptr;
}

void AOnsetPoolManager::ReleasePooledEnemy(AOnsetEnemy* Enemy)
{
	if (!Enemy || Enemy->IsPendingKillPending()) return;
	
	ReturnToPool(Enemy);
}

void AOnsetPoolManager::InitializePool()
{
	if (bPoolInitialized) return;
	bPoolInitialized = true;
	
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (int32 i = 0; i < PoolSize; i++)
	{
		AOnsetEnemy* Spawned = GetWorld()->SpawnActor<AOnsetEnemy>(AOnsetEnemy::StaticClass(), FTransform::Identity, Params);
		if (Spawned)
		{
			ReturnToPool(Spawned);                                                                  
		}
	}
	UE_LOG(LogPooling, Warning, TEXT("OnsetPoolManager: Pre-allocated %d NPCs."), ObjectPool.Num()); 

}

void AOnsetPoolManager::BeginPlay()
{
	Super::BeginPlay();
	InitializePool();	
}

void AOnsetPoolManager::ReturnToPool(AOnsetEnemy* Enemy)
{
	if (!Enemy) return;	
	if (UGroupComponent* GroupComp = Enemy->FindComponentByClass<UGroupComponent>())
	{
		GroupComp->UnregisterFromGroup();
	}
	if (AOnsetAIController* AIController = Enemy->GetController<AOnsetAIController>())
	{
		AIController->UnPossess();
		AIController->ApplyProfile(nullptr); // defensive reset — next retrieval in SpawnEnemyAtSlot will overwrite via ApplyProfile(Config.EnemyProfile)
	}
	if (!ObjectPool.Contains(Enemy)) ObjectPool.Add(Enemy);
	Enemy->ApplyProfile(nullptr); // defensive reset — next retrieval in SpawnEnemyAtSlot will overwrite via ApplyProfile(Config.EnemyProfile)
	Enemy->SetActorLocation(FVector::ZeroVector);
	Enemy->SetActorHiddenInGame(true);                                                                            
	Enemy->SetActorTickEnabled(false);                                                                            
	Enemy->DisableInput(nullptr);
	Enemy->SetActorEnableCollision(false);
}
