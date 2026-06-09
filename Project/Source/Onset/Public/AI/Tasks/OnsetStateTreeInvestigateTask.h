// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OnsetStateTreeTaskBase.h"
#include "OnsetStateTreeInvestigateTask.generated.h"

USTRUCT()       
struct FOnsetStateTreeInvestigateTaskInstanceData
{
	GENERATED_BODY()                                                                                            
                                                                                                                     
	UPROPERTY(EditAnywhere, Category = "Investigation", meta = (ClampMin = "0.0", ClampMax = "2.0"))            
	float GroupMemberSpeedMultiplier = 1.0f;                                                                    
                                                                                                                     
	UPROPERTY(EditAnywhere, Category = "Investigation", meta = (ClampMin = "0.0", ClampMax = "2.0"))            
	float NonGroupSpeedMultiplier = 0.5f;                                                                       
                                                                                                                     
	UPROPERTY(EditAnywhere, Category = "Investigation")                                                         
	float AcceptanceRadius = 100.0f;                                                                            
                                                                                                                     
	FVector InvestigationDestination = FVector::ZeroVector;
	FActiveGameplayEffectHandle SpeedEffectHandle;
};                                                                                                              
                
USTRUCT()
struct FOnsetStateTreeInvestigateTask : public FOnsetStateTreeTaskBase
{
	GENERATED_BODY()
	
	using FInstanceDataType = FOnsetStateTreeInvestigateTaskInstanceData;
	
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}
	
	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext& Context, 
		const FStateTreeTransitionResult& Transition) const override;
	
	virtual EStateTreeRunStatus Tick(
		FStateTreeExecutionContext& Context, 
		const float DeltaTime) const override;
	
	virtual void ExitState(
		FStateTreeExecutionContext& Context, 
		const FStateTreeTransitionResult& Transition) const override;
};
