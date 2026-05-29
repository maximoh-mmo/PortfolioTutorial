#include "AI/OnsetEnemy.h"
#include "Engine/World.h"
#include "Spawning/OnsetSpawner.h"

#include "Spawning/OnsetPoolManager.h"

DEFINE_LOG_CATEGORY(LogSpawner);

AOnsetSpawner::AOnsetSpawner()
{
}

void AOnsetSpawner::SpawnGroup()
{
	if (!Config.EnemyClass || Config.GroupSize <= 0) return;                                                    
	for (int32 i = 0; i < Config.GroupSize; i++)                                                                
	{    
		FTransform SpawnTransform = GetSpawnLocation(i);
		AOnsetEnemy* Spawned; 
		if (PoolManager)
		{
			Spawned = PoolManager->GetPooledEnemy();
			if (Spawned) Spawned->SetActorTransform(SpawnTransform);		
		}
		else
		{
			FActorSpawnParameters Params;                                                                           
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			Spawned = GetWorld()->SpawnActor<AOnsetEnemy>(Config.EnemyClass, SpawnTransform, Params);
		}            
		if (Spawned) SpawnedGroup.Add(Spawned);                                                                 
	}         
}

void AOnsetSpawner::DestroyGroup()
{
	for (AOnsetEnemy* Enemy : SpawnedGroup)
	{
		if (!Enemy || Enemy->IsPendingKillPending()) continue;
		if (PoolManager)
		{
			PoolManager->ReleasePooledEnemy(Enemy);
		}
		else
		{
			Enemy->Destroy();
		}
	}
	SpawnedGroup.Empty();
}

void AOnsetSpawner::BeginPlay()
{
	Super::BeginPlay();
	if (bAutoSpawn) SpawnGroup();
}

FTransform AOnsetSpawner::GetSpawnLocation(int32 Index) const
{
	if (SpawnPoints.IsValidIndex(Index) && SpawnPoints[Index])                                                  
	{                                                                                                           
		return SpawnPoints[Index]->GetActorTransform();                                                         
	}                                                                                                           
	// Fallback: ring distribution around spawner                                                               
	float Angle = (360.0f / Config.GroupSize) * Index;                                                          
	float Rad = FMath::DegreesToRadians(Angle);                                                                 
	FVector Offset = FVector(FMath::Cos(Rad), FMath::Sin(Rad), 0.0f) * Config.SpawnRadius;                      
	return FTransform(FRotator::ZeroRotator, GetActorLocation() + Offset);                         
}

void AOnsetSpawner::SpawnSingleNPC()
{
	if (!Config.EnemyClass) return;
	FTransform SpawnTransform = GetSpawnLocation(SpawnedGroup.Num());
	
	AOnsetEnemy* Spawned;
	if (PoolManager)
	{
		Spawned = PoolManager->GetPooledEnemy();
		if (Spawned) Spawned->SetActorTransform(SpawnTransform);
	}
	else
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		Spawned = GetWorld()->SpawnActor<AOnsetEnemy>(Config.EnemyClass, SpawnTransform, Params);
	}
	if (Spawned) SpawnedGroup.Add(Spawned);
}

