#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "StateTree/Tasks/OnsetStateTreeTask.h"
#include "FleeTask.generated.h"

USTRUCT()
struct FOnsetStateTreeFleeTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Flee")
	float FleeDistance = 1500.0f;

	UPROPERTY(EditAnywhere, Category = "Flee")
	float AcceptanceRadius = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Flee", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinSpeedMultiplier = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Flee", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float FleeAngleVariance = 60.0f;

	FActiveGameplayEffectHandle SpeedEffectHandle;                                                                  
	FVector FleeDestination = FVector::ZeroVector;
};

USTRUCT()
struct FFleeTask : public FOnsetStateTreeTask
{
	GENERATED_BODY()

	using FInstanceDataType = FOnsetStateTreeFleeTaskInstanceData;

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
