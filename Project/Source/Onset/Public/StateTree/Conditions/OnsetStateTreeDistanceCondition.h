// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "OnsetStateTreeDistanceCondition.generated.h"

/** Source of the distance value to compare against the threshold. */
UENUM()
enum class EOnsetStateTreeDistanceSource : uint8
{
	CurrentTarget,
	HomeLocation,
	AttackRange,
	ChaseRange
};

/** Runtime data for the distance condition. */
USTRUCT()
struct FOnsetStateTreeDistanceConditionInstance
{
	GENERATED_BODY()

	/** Which distance value to evaluate. */
	UPROPERTY(EditAnywhere, Category = Condition)
	EOnsetStateTreeDistanceSource DistanceSource = EOnsetStateTreeDistanceSource::CurrentTarget;

	/** How to compare the distance against the threshold. */
	UPROPERTY(EditAnywhere, Category = Condition)
	UE::StateTree::EComparisonOperator Comparison = UE::StateTree::EComparisonOperator::LessOrEqual;

	/** Distance value to compare against, in world units. */
	UPROPERTY(EditAnywhere, Category = Condition)
	float DistanceThreshold = 0.f;

	/** Whether a missing target evaluates as passing the condition. */
	UPROPERTY(EditAnywhere, Category = Condition)
	bool bAllowNoTarget = false;
};

USTRUCT()
struct FOnsetStateTreeDistanceCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FOnsetStateTreeDistanceConditionInstance;
	
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}
	
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
