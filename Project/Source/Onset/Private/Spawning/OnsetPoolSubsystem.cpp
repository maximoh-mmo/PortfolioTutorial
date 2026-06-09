// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawning/OnsetPoolSubsystem.h"

#include "AI/OnsetAIController.h"
#include "Components/StateTreeAIComponent.h"
#include "Enemy/GroupComponent.h"
#include "Enemy/OnsetEnemy.h"
#include "Engine/World.h"
#include "Perception/AIPerceptionComponent.h"

DEFINE_LOG_CATEGORY(LogPooling)

void UOnsetPoolSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	InitializePool();	
}

// Sets default values	
AOnsetEnemy* UOnsetPoolSubsystem::GetPooledEnemy()
{
	if (!bPoolInitialized) InitializePool();
	for (AOnsetEnemy* Enemy : ObjectPool)
	{
		if (Enemy && Enemy->IsHidden())
		{                          
			Enemy->SetActorHiddenInGame(false);                                                                   
			Enemy->SetActorTickEnabled(true);
			Enemy->SetActorEnableCollision(true);
			return Enemy;
		}
	}
	// Pool exhausted — fallback SpawnActor (hardcoded to base AOnsetEnemy)                                          
	UE_LOG(LogPooling, Warning, TEXT("OnsetPoolManager: Pool exhausted — spawning new NPC as fallback."));
	FActorSpawnParameters Params;                                                                           
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AOnsetEnemy* Enemy = GetWorld()->SpawnActor<AOnsetEnemy>(AOnsetEnemy::StaticClass(), FTransform::Identity, Params))
	{
		ReleasePooledEnemy(Enemy);
		return Enemy;                                                                                         
	}
	return nullptr;
}

AOnsetAIController* UOnsetPoolSubsystem::GetPooledController()
{
	if (!bPoolInitialized) InitializePool();
	for (AOnsetAIController* Controller : ControllerPool)
	{
		if (Controller && Controller->IsHidden())
		{         
			Controller->SetActorHiddenInGame(false);
			Controller->SetActorTickEnabled(true);
			Controller->StateTreeComponent->SetComponentTickEnabled(true);
			Controller->PerceptionComponent->SetComponentTickEnabled(true);
			return Controller;
		}
	}
	// Pool exhausted — fallback SpawnActor (hardcoded to base AOnsetEnemy)                                          
	UE_LOG(LogPooling, Warning, TEXT("OnsetPoolManager: Controller Pool exhausted — spawning new Controllers as fallback."));
	FActorSpawnParameters Params;                                                                           
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AOnsetAIController* Controller = GetWorld()->SpawnActor<AOnsetAIController>(AOnsetAIController::StaticClass(), FTransform::Identity, Params))
	{
		ReleasePooledController(Controller);
		return Controller;                                                                                         
	}
	return nullptr;
}

void UOnsetPoolSubsystem::ReleasePooledEnemy(AOnsetEnemy* Enemy)
{
	if (!Enemy || Enemy->IsPendingKillPending()) return;
	
	ReturnToPool(Enemy);
}

void UOnsetPoolSubsystem::ReleasePooledController(AOnsetAIController* Controller)
{
	if (!Controller || Controller->IsPendingKillPending()) return;
	Controller->UnPossess();
	if (!ControllerPool.Contains(Controller)) ControllerPool.Add(Controller);    
}

void UOnsetPoolSubsystem::InitializePool()
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
		AOnsetAIController* SpawnedController = GetWorld()->SpawnActor<AOnsetAIController>(AOnsetAIController::StaticClass(), FTransform::Identity, Params);
		if (SpawnedController)
		{
			ReleasePooledController(SpawnedController);
		}
	}
	
}

void UOnsetPoolSubsystem::ReturnToPool(AOnsetEnemy* Enemy)
{
	if (!Enemy) return;	
	if (UGroupComponent* GroupComp = Enemy->FindComponentByClass<UGroupComponent>())
	{
		GroupComp->UnregisterFromGroup();
	}
	if (AOnsetAIController* Controller = Enemy->GetController<AOnsetAIController>())
	{
		ReleasePooledController(Controller);
	}
	if (!ObjectPool.Contains(Enemy)) ObjectPool.Add(Enemy);
	Enemy->ApplyProfile(nullptr); // defensive reset — next retrieval in SpawnEnemyAtSlot will overwrite via ApplyProfile(Config.EnemyAIProfile)
	Enemy->OwningSpawner = nullptr;
	Enemy->SetActorLocation(FVector::ZeroVector);
	Enemy->SetActorHiddenInGame(true);                                                                            
	Enemy->SetActorTickEnabled(false);                                                                            
	Enemy->DisableInput(nullptr);
	Enemy->SetActorEnableCollision(false);
}
