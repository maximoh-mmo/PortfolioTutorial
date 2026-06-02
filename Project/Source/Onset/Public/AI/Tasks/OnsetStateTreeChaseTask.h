#pragma once

#include "CoreMinimal.h"
#include "OnsetStateTreeTaskBase.h"
#include "StateTreeTaskBase.h"
#include "OnsetStateTreeChaseTask.generated.h"

USTRUCT()
struct FOnsetStateTreeChaseTaskInstanceData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "Chase")
	float AcceptanceRadius = 50.0f;
};

USTRUCT()
struct FOnsetStateTreeChaseTask : public FOnsetStateTreeTaskBase
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
