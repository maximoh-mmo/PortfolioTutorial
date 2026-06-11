#pragma once
#include "StateTreeConditionBase.h"
#include "OnsetStateTreeFleeCondition.generated.h"

USTRUCT()
struct FOnsetFleeConditionInstanceData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "Flee")
	float GroupCouragePerAlly = 0.05f;
	
	UPROPERTY(EditAnywhere, Category = "Flee", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FleeProbability = 0.7f;
	
	UPROPERTY(EditAnywhere,Category = "Flee")
	float GroupSupportRadius = 800.0f;
};

USTRUCT()
struct FOnsetStateTreeFleeCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()
	
	using FInstanceDataType = FOnsetFleeConditionInstanceData;
	
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}
	
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;	
};
