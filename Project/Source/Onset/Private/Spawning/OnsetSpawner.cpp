#include "Spawning/OnsetSpawner.h"

#include "AI/AIProfile.h"
#include "AI/OnsetAIController.h"
#include "Enemy/OnsetEnemy.h"
#include "Engine/World.h"
#include "Spawning/GroupManagerComponent.h"
#include "Spawning/OnsetPoolManager.h"
#include "Spawning/SpawnerSlot.h"

DEFINE_LOG_CATEGORY(LogSpawner);

AOnsetSpawner::AOnsetSpawner()
{
	GroupManager = CreateDefaultSubobject<UGroupManagerComponent>(TEXT("GroupManager"));
}

void AOnsetSpawner::SpawnGroup()
{
	if (Config.EnemyProfile == nullptr || Config.GroupSize <= 0) return;     
	if (Slots.Num() == 0) InitSlots();                                                                          
                                                                                                                     
	for (int32 i = 0; i < Slots.Num(); i++)                                                                     
	{                                                                                                           
		FSpawnerSlot& Slot = Slots[i];                                                                          
		if (Slot.Occupant && !Slot.Occupant->IsPendingKillPending())                                            
			continue;                                                                                           
                                                                                                                     
		SpawnEnemyAtSlot(i);                                                                                       
	}                
}

void AOnsetSpawner::DestroyGroup()
{
	for (int32 i = 0; i < Slots.Num(); i++)
	{
		if (Slots[i].Occupant == nullptr || Slots[i].Occupant->IsPendingKillPending()) continue; 
		if (GroupManager) GroupManager->UnregisterMember(Slots[i].Occupant);
		if (PoolManager)
		{
			PoolManager->ReleasePooledEnemy(Slots[i].Occupant);
		}
		else
		{
			
			Slots[i].Occupant->Destroy();
		}
		Slots[i].Occupant = nullptr;
	}
}

void AOnsetSpawner::BeginPlay()
{
	Super::BeginPlay();                                                                                
	InitSlots();                                                                                                
	if (bAutoSpawn) SpawnGroup();          
}

void AOnsetSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AOnsetSpawner::InitSlots()
{
	int32 Count = FMath::Max(1, Config.GroupSize);
	Slots.SetNum(Count);
	
	for (int32 i = 0; i < Count; i++)
	{
		FSpawnerSlot& Slot = Slots[i];
		Slot.Occupant = nullptr;
		
		if (SpawnPoints.IsValidIndex(i) && SpawnPoints[i])
		{
			Slot.SpawnTransform = SpawnPoints[i]->GetActorTransform();
		}
		else
		{
			float Angle = (360.0f / Count) * i;
			float Rad = FMath::DegreesToRadians(Angle);
			FVector Offset = FVector(FMath::Cos(Rad), FMath::Sin(Rad), 0.0f) * Config.SpawnRadius;
			Slot.SpawnTransform = FTransform(FRotator::ZeroRotator, GetActorLocation() + Offset);
		}
	}
}

AOnsetEnemy* AOnsetSpawner::SpawnEnemyAtSlot(int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex)) return nullptr;
	FSpawnerSlot& Slot = Slots[SlotIndex];
	if (Slot.Occupant && IsValid(Slot.Occupant)) return nullptr; // already occupied
	
	AOnsetEnemy* Spawned = nullptr;
	if (PoolManager)
	{
		Spawned = PoolManager->GetPooledEnemy();
		if (Spawned) Spawned->SetActorTransform(Slot.SpawnTransform);
	}
	else
	{
		UE_LOG(LogSpawner, Warning, TEXT("SpawnEnemyAtSlot: PoolManager is null — cannot spawn NPC."));
	}
	if (Spawned)
	{
		Spawned->ApplyProfile(Config.EnemyProfile);
		AOnsetAIController* AIController = PoolManager->GetPooledController();
		if (!AIController)
		{
			UE_LOG(LogSpawner, Error, TEXT("SpawnEnemyAtSlot: No pooled controller available."));
			return nullptr;
		}
		
		AIController->ApplyProfile(Config.EnemyProfile);
		AIController->Possess(Spawned);
		Slot.Occupant = Spawned;
		if (GroupManager) GroupManager->RegisterMember(Spawned);
	}
	return Spawned;
}

void AOnsetSpawner::SpawnSingleNPC()
{
	// get first available Free spawn Transform
	for (auto Slot : Slots)
	{
		if (!Slot.Occupant || !IsValid(Slot.Occupant)) 
		{
			SpawnEnemyAtSlot(&Slot - Slots.GetData()); // pointer arithmetic to get index
			return;
		}
	}
}

void AOnsetSpawner::DebugKillAll()
{
	DestroyGroup();
}

void AOnsetSpawner::DebugKillLast()
{
	for (int32 i = Slots.Num() - 1; i >= 0; i--)
	{
		if (Slots[i].Occupant && IsValid(Slots[i].Occupant))
		{
			if (GroupManager) GroupManager->UnregisterMember(Slots[i].Occupant);
			if (PoolManager)
			{
				PoolManager->ReleasePooledEnemy(Slots[i].Occupant);
			}
			else Slots[i].Occupant->Destroy();
			Slots[i].Occupant = nullptr;
			return;
		}
	}
}
