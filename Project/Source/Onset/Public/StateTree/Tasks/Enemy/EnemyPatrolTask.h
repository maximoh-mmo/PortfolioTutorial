#pragma once

#include "CoreMinimal.h"
#include "StateTree/Tasks/OnsetStateTreeTask.h"
#include "EnemyPatrolTask.generated.h"

USTRUCT()
struct FOnsetStateTreePatrolInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Patrol")
	float MinIdleDuration = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Patrol")
	float MaxIdleDuration = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Patrol")
	float RoamRadius = 500.0f;

	UPROPERTY(EditAnywhere, Category = "Patrol")
	float RoamChance = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Patrol")
	float AcceptanceRadius = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Patrol")
	float PauseOnArrival = 2.0f;

	bool bIsRoaming = false;
	bool bHasArrived = false;
	float RemainingTime = 0.0f;
	float PauseTimer = 0.0f;
	FVector Destination = FVector::ZeroVector;
};

USTRUCT()
struct FEnemyPatrolTask : public FOnsetStateTreeTask
{
	GENERATED_BODY()

	using FInstanceDataType = FOnsetStateTreePatrolInstanceData;

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
