// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTree/Tasks/OnsetStateTreeTask.h"
#include "EnemyRoamTask.generated.h"

/** Runtime data for the enemy roam task. */
USTRUCT()
struct FOnsetStateTreeRoamInstanceData
{
	GENERATED_BODY()

	/** Radius from home location to pick random destinations. */
	UPROPERTY(EditAnywhere, Category="Roam")
	float RoamRadius = 600.0f;

	/** Distance threshold for arriving at the destination. */
	UPROPERTY(EditAnywhere, Category="Roam")
	float AcceptanceRadius = 50.0f;

	/** Time in seconds to pause after reaching a destination before picking the next. */
	UPROPERTY(EditAnywhere, Category="Roam")
	float PauseOnArrival = 1.0f;

	/** Current movement destination. */
	FVector Destination = FVector::ZeroVector;
	/** Remaining pause time before the next roam point is selected. */
	float PauseTimer = 0.0f;
	/** Whether the NPC has arrived at the current destination. */
	bool bHasArrived = false;
};

USTRUCT()
struct FEnemyRoamTask : public FOnsetStateTreeTask
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