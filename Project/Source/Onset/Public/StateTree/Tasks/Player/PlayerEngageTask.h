#pragma once
#include "StateTree/Tasks/OnsetStateTreeTask.h"
#include "PlayerEngageTask.generated.h"

USTRUCT()                                                                                                                                                                    
struct FPlayerEngageTaskInstanceData                                                                                                                                         
{                                                                                                                                                                            
	GENERATED_BODY()                                                                                                                                                         
	UPROPERTY(EditAnywhere, Category = "Engage")                                                                
	float AcceptanceRadius = 50.0f;                                                                             
	UPROPERTY(EditAnywhere, Category = "Engage")                                                                
	float MaxEngageDuration = 8.0f;                                                                             
	UPROPERTY(EditAnywhere, Category = "Engage")                                                                
	float AoEOverlapRadius = 400.0f;                                                                            
	UPROPERTY(EditAnywhere, Category = "Engage")                                                                
	float AttackTickInterval = 0.25f;                                                                            
                                                                                                                      
	float EngageStartTime = 0.0f;                                                                               
	float LastAttackTick = 0.0f;
};                    

USTRUCT()
struct FPlayerEngageTask : public FOnsetStateTreeTask
{
	GENERATED_BODY()
	
	using FInstanceDataType = FPlayerEngageTaskInstanceData;
	
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}
	
	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	
	virtual EStateTreeRunStatus Tick(
		FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	
	virtual void ExitState(
		FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
