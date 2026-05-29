#include "AI/OnsetEnemy.h"
#include "Engine/World.h"
#include "Spawning/OnsetSpawner.h"

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
		FActorSpawnParameters Params;                                                                           
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;                                                                                 
		AActor* Spawned = GetWorld()->SpawnActor<AActor>(Config.EnemyClass, SpawnTransform, Params);            
		if (Spawned) SpawnedGroup.Add(Spawned);                                                                 
	}         
}

void AOnsetSpawner::DestroyGroup()
{
	for (AActor* Actor : SpawnedGroup)
	{
		if (Actor && !Actor->IsPendingKillPending()) Actor->Destroy();
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
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AActor* Spawned = GetWorld()->SpawnActor<AActor>(Config.EnemyClass, SpawnTransform, Params);
	if (Spawned) SpawnedGroup.Add(Spawned);
}

