#pragma once
#include "StateTree/Tasks/OnsetStateTreeTask.h"
#include "PlayerAttackTask.generated.h"

USTRUCT()                                                                                                                                                                    
struct FPlayerAttackTaskInstanceData                                                                                                                                         
{                                                                                                                                                                            
	GENERATED_BODY()                                                                                                                                                         
	UPROPERTY(EditAnywhere, Category = "Attack")                                                                                                                             
	float TickInterval = 0.25f;                                                                                                                                              
	float LastTickTime = 0.0f;                                                                                                                                               
	// For tracking cooldown-aware ability selection                                                                                                                         
};                    

USTRUCT()
struct FPlayerAttackTask : public FOnsetStateTreeTask
{
	GENERATED_BODY()
	
	using FInstanceDataType = FPlayerAttackTaskInstanceData;
	
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}
	
	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	
	virtual EStateTreeRunStatus Tick(
		FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
