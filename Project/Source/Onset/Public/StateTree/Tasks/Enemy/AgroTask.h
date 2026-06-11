// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTree/Tasks/OnsetStateTreeTask.h"
#include "AgroTask.generated.h"

USTRUCT()                                                                                                       
struct FOnsetStateTreeAgroInstanceData                                                                          
{                                                                                                               
	GENERATED_BODY()                                                                                            
                                                                                                                     
	UPROPERTY(EditAnywhere, Category = "Agro")                                                                  
	float TurnSpeed = 360.0f;                                                                                   
                                                                                                                     
	UPROPERTY(EditAnywhere, Category = "Agro")                                                                  
	float FacingThreshold = 15.0f;                                                                              
                                                                                                                     
	UPROPERTY(EditAnywhere, Category = "Agro")                                                                  
	float MinDuration = 0.5f;                                                                                   
                                                                                                                     
	float TimeSpent = 0.0f;                                                                                     
};

USTRUCT()
struct FAgroTask : public FOnsetStateTreeTask
{
	GENERATED_BODY()

	using FInstanceDataType = FOnsetStateTreeAgroInstanceData;
	
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
};
