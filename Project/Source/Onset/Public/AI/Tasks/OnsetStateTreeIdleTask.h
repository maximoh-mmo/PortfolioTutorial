// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "OnsetStateTreeIdleTask.generated.h"

USTRUCT()
struct FOnsetStateTreeIdleInstanceData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category="Idle")
	float MinDuration = 3.0f;
	
	UPROPERTY(EditAnywhere, Category="Idle")
	float MaxDuration = 10.0f;
	
	float RemainingDuration = 0.0f;
	
};

USTRUCT()
struct FOnsetStateTreeIdleTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FOnsetStateTreeIdleInstanceData;
	
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
