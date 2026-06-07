#pragma once
#include "StateTreeConditionBase.h"
#include "OnsetStateTreeHearingCondition.generated.h"

USTRUCT()
struct FOnsetStateTreeHearingConditionInstanceData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "Investigation")                                                             
	float MaxTimeSinceLastNoise = 5.0f;      
};


USTRUCT()
struct FOnsetStateTreeHearingCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()
	
	using FInstanceDataType = FOnsetStateTreeHearingConditionInstanceData;
	
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}
	
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
