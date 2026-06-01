// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "OnsetStateTreeLostTargetTask.generated.h"

USTRUCT()
struct FOnsetStateTreeLostTargetInstanceData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "LostTarget")
	float MinDuration = 2.0f;
	UPROPERTY(EditAnywhere, Category = "LostTarget")
	float MaxDuration = 4.0f;
	
	float RemainingTime = 0.0f;
	
};

USTRUCT()
struct FOnsetStateTreeLostTargetTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FOnsetStateTreeLostTargetInstanceData;
	
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