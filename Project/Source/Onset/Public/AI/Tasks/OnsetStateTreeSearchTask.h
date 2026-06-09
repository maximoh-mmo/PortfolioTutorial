// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OnsetStateTreeTaskBase.h"
#include "OnsetStateTreeSearchTask.generated.h"

/**
 * 
 */
USTRUCT()
struct FOnsetStateTreeSearchInstanceData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "Search")
	float SearchRadius = 300.0f;
	
	UPROPERTY(EditAnywhere, Category = "Search", meta = (ClampMin = "0", ClampMax = "180"))
	float ConeHalfAngle = 120.0f;
	
	UPROPERTY(EditAnywhere, Category = "Search", meta = (ClampMin = "1", ClampMax = "10"))
	int32 MinCycles = 3;
	
	UPROPERTY(EditAnywhere, Category = "Search")
	float MinSearchDuration = 10.0f;
	
	UPROPERTY(EditAnywhere, Category = "Search", meta = (ClampMin = "0", ClampMax = "2"))
	float SearchMovementSpeedMultiplier = 0.5f;
	
	UPROPERTY(EditAnywhere, Category = "Search", meta = (ClampMin = "0", ClampMax = "150"))
	float AcceptanceRadius = 50.0f;
	
	// Runtime
	FVector SearchCenter = FVector(0.0f, 0.0f, 0.0f);
	FVector InitialForward = FVector(0.0f, 0.0f, 0.0f);
	FVector CurrentSearchPoint = FVector(0.0f, 0.0f, 0.0f);
	FActiveGameplayEffectHandle SpeedEffectHandle;
	int32 CurrentCycle = 0;
	float  ElapsedTime = 0.0f;
};

USTRUCT()
struct FOnsetStateTreeSearchTask : public FOnsetStateTreeTaskBase
{
	GENERATED_BODY()
	
	using FInstanceDataType = FOnsetStateTreeSearchInstanceData;
	
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
	
	FVector PickSearchPoint(
		const FInstanceDataType& InstanceData, const FVector& Vector, UWorld* World) const;
	void ApplyYawSweep(const FStateTreeExecutionContext& Context, AOnsetAIController* Controller, AOnsetBaseCharacter* Self) const;
};
