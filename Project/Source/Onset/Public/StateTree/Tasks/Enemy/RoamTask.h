// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTree/Tasks/OnsetStateTreeTask.h"
#include "RoamTask.generated.h"

USTRUCT()
struct FOnsetStateTreeRoamInstanceData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category="Roam")
	float RoamRadius = 600.0f;
		
	UPROPERTY(EditAnywhere, Category="Roam")
	float AcceptanceRadius = 50.0f;
	
	UPROPERTY(EditAnywhere, Category="Roam")
	float PauseOnArrival = 1.0f;
	
	FVector Destination = FVector::ZeroVector;
	float PauseTimer = 0.0f;
	bool bHasArrived = false;
};

USTRUCT()
struct FRoamTask : public FOnsetStateTreeTask
{
	GENERATED_BODY()

	using FInstanceDataType = FOnsetStateTreeRoamInstanceData;
	
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