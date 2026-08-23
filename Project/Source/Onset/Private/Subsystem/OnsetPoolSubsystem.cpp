// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystem/OnsetPoolSubsystem.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "AI/OnsetAIController.h"
#include "Components/StateTreeAIComponent.h"
#include "Enemy/GroupComponent.h"
#include "Enemy/OnsetEnemy.h"
#include "Engine/World.h"
#include "Perception/AIPerceptionComponent.h"
#include "Subsystem/OnsetThreatSubsystem.h"

DEFINE_LOG_CATEGORY(LogPooling)

void UOnsetPoolSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (InWorld.GetNetMode() == NM_Client) return;
	InitializePool();	
}

AOnsetEnemy* UOnsetPoolSubsystem::GetPooledEnemy()
{
	if (!GetWorld() || GetWorld()->GetNetMode() == NM_Client) return nullptr;
	if (!bPoolInitialized) InitializePool();
	for (AOnsetEnemy* Enemy : ObjectPool)
	{
		if (Enemy && Enemy->IsHidden())
		{
			UE_LOG(LogTemp, Warning, TEXT("[DIAG] GetPooledEnemy: pooled hit %s"), *Enemy->GetName());
			ActiveEnemies.Add(Enemy);
			Enemy->OnRespawn();
			return Enemy;
		}
	}
	// Pool exhausted — grow the pool: spawn a fresh enemy, register it directly in
	// ObjectPool (no teardown pass needed on a brand-new actor), and normalize its
	// state to match what pooled retrievals return via OnRespawn.
	UE_LOG(LogPooling, Warning, TEXT("UOnsetPoolSubsystem: Enemy pool exhausted — spawning new NPC as fallback."));
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AOnsetEnemy* Enemy = GetWorld()->SpawnActor<AOnsetEnemy>(AOnsetEnemy::StaticClass(), FTransform::Identity, Params))
	{
		if (!ObjectPool.Contains(Enemy)) ObjectPool.Add(Enemy);
		ActiveEnemies.Add(Enemy);
		// Parity with pre-allocated members + the pooled-retrieval path: attributes
		// initialized, un-hidden, ticking, colliding before SpawnEnemyAtSlot takes over.
		Enemy->ResetAttributes();
		Enemy->OnRespawn();
		return Enemy;
	}
	return nullptr;
}

AOnsetAIController* UOnsetPoolSubsystem::GetPooledController()
{
	if (!GetWorld() || GetWorld()->GetNetMode() == NM_Client) return nullptr;
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
	// Pool exhausted — grow the pool: spawn a fresh controller, register it directly in
	// ControllerPool, and restore the active state that pooled retrievals set explicitly
	// (UnPossess inside ReleasePooledController would leave it hidden with ticks off).
	UE_LOG(LogPooling, Warning, TEXT("UOnsetPoolSubsystem: Controller pool exhausted — spawning new controller as fallback."));
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AOnsetAIController* Controller = GetWorld()->SpawnActor<AOnsetAIController>(AOnsetAIController::StaticClass(), FTransform::Identity, Params))
	{
		if (!ControllerPool.Contains(Controller)) ControllerPool.Add(Controller);
		Controller->SetActorHiddenInGame(false);
		Controller->SetActorTickEnabled(true);
		Controller->StateTreeComponent->SetComponentTickEnabled(true);
		Controller->PerceptionComponent->SetComponentTickEnabled(true);
		return Controller;
	}
	return nullptr;
}

void UOnsetPoolSubsystem::ReleasePooledEnemy(AOnsetEnemy* Enemy)
{
	if (!GetWorld() || GetWorld()->GetNetMode() == NM_Client) return;
	if (!Enemy || !IsValid(Enemy)) return;
	
	ReturnToPool(Enemy);
}

void UOnsetPoolSubsystem::ReleasePooledController(AOnsetAIController* Controller)
{
	if (!GetWorld() || GetWorld()->GetNetMode() == NM_Client) return;
	if (!Controller || !IsValid(Controller)) return;
	Controller->UnPossess();
	if (!ControllerPool.Contains(Controller)) ControllerPool.Add(Controller);    
}

void UOnsetPoolSubsystem::InitializePool()
{
	if (!GetWorld() || GetWorld()->GetNetMode() == NM_Client) return;
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
	if (!GetWorld() || GetWorld()->GetNetMode() == NM_Client) return;
	if (!Enemy) return;	
	if (UOnsetThreatSubsystem* ThreatSub = GetWorld()->GetSubsystem<UOnsetThreatSubsystem>())                       
	{                                                                                                               
		ThreatSub->RemoveEnemy(Enemy);                                                                              
	}   
	if (ActiveEnemies.Contains(Enemy)) ActiveEnemies.Remove(Enemy);
	if (UGroupComponent* GroupComp = Enemy->FindComponentByClass<UGroupComponent>())
	{
		GroupComp->UnregisterFromGroup();
	}
	if (AOnsetAIController* Controller = Enemy->GetController<AOnsetAIController>())
	{
		ReleasePooledController(Controller);
	}
	if (Enemy->AbilitySystemComponent)                                                                              
	{                                                                                                               
		Enemy->AbilitySystemComponent->RemoveActiveEffects(FGameplayEffectQuery(), -1);  
	}    
	Enemy->ResetAttributes();
	// defensive reset — next retrieval in SpawnEnemyAtSlot will overwrite via ApplyProfile(Config.EnemyAIProfile)
	Enemy->ApplyProfile(nullptr);
	Enemy->OwningSpawner = nullptr;
	Enemy->SetActorLocation(FVector::ZeroVector);
	Enemy->SetActorHiddenInGame(true);                                                                            
	Enemy->SetActorTickEnabled(false);                                                    
	Enemy->DisableInput(nullptr);
	Enemy->SetActorEnableCollision(false);
	if (!ObjectPool.Contains(Enemy)) ObjectPool.Add(Enemy);
}
