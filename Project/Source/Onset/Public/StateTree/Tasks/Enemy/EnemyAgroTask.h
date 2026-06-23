// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTree/Tasks/OnsetStateTreeTask.h"
#include "EnemyAgroTask.generated.h"

/** Runtime data for the enemy agro task. */
USTRUCT()
struct FOnsetStateTreeAgroInstanceData
{
	GENERATED_BODY()

	/** Rotation speed when facing the target, in degrees per second. */
	UPROPERTY(EditAnywhere, Category = "Agro")
	float TurnSpeed = 360.0f;

	/** Angular threshold in degrees before the NPC is considered facing the target. */
	UPROPERTY(EditAnywhere, Category = "Agro")
	float FacingThreshold = 15.0f;

	/** Minimum time to remain in the agro state. */
	UPROPERTY(EditAnywhere, Category = "Agro")
	float MinDuration = 0.5f;

	/** Time spent in the agro state. */
	float TimeSpent = 0.0f;
};

USTRUCT()
struct FEnemyAgroTask : public FOnsetStateTreeTask
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
