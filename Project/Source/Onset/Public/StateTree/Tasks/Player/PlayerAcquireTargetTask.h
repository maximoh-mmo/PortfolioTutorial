#pragma once
#include "StateTree/Tasks/OnsetStateTreeTask.h"
#include "PlayerAcquireTargetTask.generated.h"

USTRUCT()                                                                                                                                                                    
struct FPlayerAcquireTargetTaskInstanceData                                                                                                                                  
{                                                                                                                                                                            
	GENERATED_BODY()                                                                                                                                                         
	UPROPERTY(EditAnywhere, Category = "Acquire")                                                                                                                            
	float SearchInterval = 0.5f;                                                                                                                                             
	float LastSearchTime = 0.0f;                                                                                                                                             
};                                                                                                                                                                           
                                                                                                                                                                                  
USTRUCT()       
struct FPlayerAcquireTargetTask : public FOnsetStateTreeTask
{
	GENERATED_BODY()
	
	// Tick-based: every SearchInterval, scan ActiveEnemies via pool                                                                                                         
	// Filter by MaxDistance (leash from home) and Aggression (scan range)                                                                                                   
	// Set TargetingComponent->SetTarget(best)                                                                                                                               
	// Returns Succeeded if target found, Failed if none   
};
