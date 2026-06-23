// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "StateTreeConditionBase.h"
#include "OnsetStateTreeHearingCondition.generated.h"

/** Runtime data for the hearing condition. */
USTRUCT()
struct FOnsetStateTreeHearingConditionInstanceData
{
	GENERATED_BODY()

	/** Maximum time since the last noise event for the condition to pass. */
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
