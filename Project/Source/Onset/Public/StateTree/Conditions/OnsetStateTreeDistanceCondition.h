#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "Animation/AnimNode_TransitionPoseEvaluator.h"
#include "OnsetStateTreeDistanceCondition.generated.h"

UENUM()
enum class EOnsetStateTreeDistanceSource : uint8
{
	CurrentTarget,
	HomeLocation,
	AttackRange,
	ChaseRange
};

USTRUCT()
struct FOnsetStateTreeDistanceConditionInstance
{
	GENERATED_BODY()
	
	   
	UPROPERTY(EditAnywhere, Category = Condition)
	EOnsetStateTreeDistanceSource DistanceSource = EOnsetStateTreeDistanceSource::CurrentTarget;
	
	UPROPERTY(EditAnywhere, Category = Condition)
	UE::StateTree::EComparisonOperator Comparison = UE::StateTree::EComparisonOperator::LessOrEqual;
	
	UPROPERTY(EditAnywhere, Category = Condition)
	float DistanceThreshold = 0.f;
	
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
