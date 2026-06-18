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
	
	using FInstanceDataType = FPlayerAcquireTargetTaskInstanceData;
	
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct(); 
	}
	
	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(
		FStateTreeExecutionContext& Context, const float DeltaTime) const override;   
};
