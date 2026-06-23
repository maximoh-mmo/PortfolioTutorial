// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTree/Tasks/OnsetStateTreeTask.h"
#include "EnemySearchTask.generated.h"

/** Runtime data for the enemy search task. */
USTRUCT()
struct FOnsetStateTreeSearchInstanceData
{
	GENERATED_BODY()

	/** Radius around the search origin to wander. */
	UPROPERTY(EditAnywhere, Category = "Search")
	float SearchRadius = 300.0f;

	/** Half-angle of the forward-facing search cone, in degrees. */
	UPROPERTY(EditAnywhere, Category = "Search", meta = (ClampMin = "0", ClampMax = "180"))
	float ConeHalfAngle = 120.0f;

	/** Minimum number of search cycles before completing. */
	UPROPERTY(EditAnywhere, Category = "Search", meta = (ClampMin = "1", ClampMax = "10"))
	int32 MinCycles = 3;

	/** Minimum total duration of the search state. */
	UPROPERTY(EditAnywhere, Category = "Search")
	float MinSearchDuration = 10.0f;

	/** Movement speed multiplier applied while searching. */
	UPROPERTY(EditAnywhere, Category = "Search", meta = (ClampMin = "0", ClampMax = "2"))
	float SearchMovementSpeedMultiplier = 0.5f;

	/** Distance threshold for arriving at a search point. */
	UPROPERTY(EditAnywhere, Category = "Search", meta = (ClampMin = "0", ClampMax = "150"))
	float AcceptanceRadius = 50.0f;

	/** Origin point of the current search. */
	FVector SearchCenter = FVector(0.0f, 0.0f, 0.0f);
	/** Forward direction when the search started. */
	FVector InitialForward = FVector(0.0f, 0.0f, 0.0f);
	/** Current destination point the NPC is moving toward. */
	FVector CurrentSearchPoint = FVector(0.0f, 0.0f, 0.0f);
	/** Handle to the active speed-modifier gameplay effect. */
	FActiveGameplayEffectHandle SpeedEffectHandle;
	/** Current search cycle index. */
	int32 CurrentCycle = 0;
	/** Time spent in the search state. */
	float ElapsedTime = 0.0f;
};

USTRUCT()
struct FEnemySearchTask : public FOnsetStateTreeTask
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
		const FInstanceDataType& InstanceData, const FVector& Origin, UWorld* World) const;
	void ApplyYawSweep(const FStateTreeExecutionContext& Context, AOnsetAIController* Controller, AOnsetBaseCharacter* Self) const;
};
