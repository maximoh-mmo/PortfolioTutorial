#pragma once

#include "CoreMinimal.h"
#include "StateTree/Tasks/OnsetStateTreeTask.h"
#include "ChaseTask.generated.h"

USTRUCT()
struct FOnsetStateTreeChaseTaskInstanceData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "Chase")
	float AcceptanceRadius = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Chase")
	float MaxChaseDuration = 3.0f;
	
	UPROPERTY(EditAnywhere, Category = "Chase")
	float SpreadRadius = 280.0f;
	
	FVector OffsetLocation = FVector::ZeroVector;
	float ChaseStartTime = 0.0f;
};

USTRUCT()
struct FChaseTask : public FOnsetStateTreeTask
{
	GENERATED_BODY()

	using FInstanceDataType = FOnsetStateTreeChaseTaskInstanceData;
	
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
