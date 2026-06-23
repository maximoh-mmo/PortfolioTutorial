#include "StateTree/Tasks/Player/PlayerAcquireTargetTask.h"

#include "NavigationSystem.h"
#include "StateTreeExecutionContext.h"
#include "Enemy/OnsetEnemy.h"
#include "Engine/World.h"
#include "Player/OnsetPlayerCharacter.h"
#include "Spawning/OnsetPoolSubsystem.h"

/**
	- Get UOnsetPoolSubsystem from world                                                                                                                                         
	- Iterate GetActiveEnemies()                                                                                                                                                 
	- Score each: DistToHome <= MaxDistance && DistToSelf <= AcquireRange                                                                                                        
	- Pick closest valid target                                                                                                                                                  
	- Set target on TargetingComponent   
*/

static bool BestAvailableTarget(const FStateTreeExecutionContext& Context)                                           
{
	if (FOnsetStateTreeTask::GetTarget(Context)) return true;
	
	AOnsetPlayerAIController* Controller = FOnsetStateTreeTask::GetPlayerController(Context);
	if (!Controller) return false;
	
	AOnsetPlayerCharacter* Self = Cast<AOnsetPlayerCharacter>(Controller->GetPawn());                                   
    if (!Self) return false;

	const UOnsetPoolSubsystem* Pool = Controller->GetWorld()->GetSubsystem<UOnsetPoolSubsystem>();
	if (!Pool) return false;

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Controller->GetWorld());                                         
	if (!NavSys) return false;
	
	// MaxDistance/ Aggression are 0-1 lerp ratios, not absolute distances
	const float Leash = FMath::Lerp(Controller->MinLeash, Controller->MaxLeash, Controller->MaxDistance);
	const float Range = FMath::Lerp(Controller->MinAcquire, Controller->MaxAcquire, Controller->Aggression);
	const FVector Home = Self->HomeTransform.GetLocation();
	const FVector SelfLoc = Self->GetActorLocation();
	
	AOnsetEnemy* Best = nullptr;                                                                                      
	float BestDist = FLT_MAX;                                                                                            
	for (const TWeakObjectPtr<AOnsetEnemy>& Weak : Pool->GetActiveEnemies())
	{
		AOnsetEnemy* Enemy = Weak.Get();                                                                                     
		if (!Enemy || !Enemy->IsAlive()) continue;                                                                                                                                                                                                    
		float DistH = FVector::Dist(Enemy->GetActorLocation(), Home);                                                        
		if (DistH > Leash) continue;                                                                                                                                                                                                              
		float DistS = FVector::Dist(Enemy->GetActorLocation(), SelfLoc);                                                     
		if (DistS > Range) continue;
		
		FNavLocation Projection;
		if (!NavSys->ProjectPointToNavigation(Enemy->GetActorLocation(), Projection, FVector(200.0f))) continue;
		if (DistS < BestDist) { BestDist = DistS; Best = Enemy; }
	}
	if (Best)                                                                                                   
    {
		FOnsetStateTreeTask::SetTarget(Context, Best);                                                          
		return true;                                                                                            
    }                                                                                                           
	return false;                                                                                               
}                                 

EStateTreeRunStatus FPlayerAcquireTargetTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	if (BestAvailableTarget(Context)) return EStateTreeRunStatus::Succeeded;
	
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	AOnsetPlayerAIController* Controller = GetPlayerController(Context);
	if (!Controller) return EStateTreeRunStatus::Failed;
	
	InstanceData.LastSearchTime = Controller->GetWorld()->GetTimeSeconds();
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FPlayerAcquireTargetTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	AOnsetPlayerAIController* Controller = GetPlayerController(Context);
	if (!Controller) return EStateTreeRunStatus::Failed;
	
	float Now = Controller->GetWorld()->GetTimeSeconds();
	if (Now - InstanceData.LastSearchTime < InstanceData.SearchInterval) return EStateTreeRunStatus::Running;
	
	InstanceData.LastSearchTime = Now;
	return BestAvailableTarget(Context) 
		? EStateTreeRunStatus::Succeeded 
		: EStateTreeRunStatus::Running;	
}
