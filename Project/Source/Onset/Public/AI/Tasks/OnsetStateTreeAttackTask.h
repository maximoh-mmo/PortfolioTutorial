#pragma once

#include "CoreMinimal.h"
#include "OnsetStateTreeTaskBase.h"
#include "OnsetStateTreeAttackTask.generated.h"

USTRUCT()
struct FOnsetStateTreeAttackTaskInstanceData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "Attack")
	float CooldownDuration = 1.5f;
	
	float RemainingCooldown = 0.0f;
};


USTRUCT()
struct FOnsetStateTreeAttackTask : public FOnsetStateTreeTaskBase
{
	GENERATED_BODY()
	
	using FInstanceDataType = FOnsetStateTreeAttackTaskInstanceData; 
	
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
