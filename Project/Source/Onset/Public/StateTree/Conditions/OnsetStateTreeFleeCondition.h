// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "StateTreeConditionBase.h"
#include "OnsetStateTreeFleeCondition.generated.h"

/** Runtime data for the flee condition. */
USTRUCT()
struct FOnsetFleeConditionInstanceData
{
	GENERATED_BODY()

	/** Courage bonus per alive group member within the support radius. */
	UPROPERTY(EditAnywhere, Category = "Flee")
	float GroupCouragePerAlly = 0.05f;

	/** Base probability of fleeing when the condition is evaluated. */
	UPROPERTY(EditAnywhere, Category = "Flee", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FleeProbability = 0.7f;

	/** Radius in which alive group members are counted for courage calculation. */
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
