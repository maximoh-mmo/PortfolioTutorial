// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTree/Tasks/OnsetStateTreeTask.h"
#include "EnemyInvestigateTask.generated.h"

/** Runtime data for the enemy investigate task. */
USTRUCT()
struct FOnsetStateTreeInvestigateTaskInstanceData
{
	GENERATED_BODY()

	/** Movement speed multiplier for group members during investigation. */
	UPROPERTY(EditAnywhere, Category = "Investigation", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float GroupMemberSpeedMultiplier = 1.0f;

	/** Movement speed multiplier for non-group members during investigation. */
	UPROPERTY(EditAnywhere, Category = "Investigation", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float NonGroupSpeedMultiplier = 0.5f;

	/** Distance threshold for arriving at the investigation point. */
	UPROPERTY(EditAnywhere, Category = "Investigation")
	float AcceptanceRadius = 100.0f;

	/** Destination point being investigated. */
	FVector InvestigationDestination = FVector::ZeroVector;
	/** Handle to the active speed-modifier gameplay effect. */
	FActiveGameplayEffectHandle SpeedEffectHandle;
};
                
USTRUCT()
struct FEnemyInvestigateTask : public FOnsetStateTreeTask
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
