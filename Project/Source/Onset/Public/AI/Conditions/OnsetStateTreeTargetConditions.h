#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "AI/Tasks/OnsetStateTreeTaskBase.h"
#include "OnsetStateTreeTargetConditions.generated.h"

USTRUCT()                                                                                                       
struct FOnsetStateTreeTargetConditionInstanceData                                                            
{                                                                                                               
	GENERATED_BODY()                                                                                            
};     

USTRUCT()
struct FOnsetStateTreeTargetConditions : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()
	
	using FInstanceDataType = FOnsetStateTreeTargetConditionInstanceData;
	
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}
	
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override
	{
		return FOnsetStateTreeTaskBase::GetTarget(Context) != nullptr;
	}
};

USTRUCT()                                                                                                       
struct FOnsetStateTreeHasNoTargetCondition : public FStateTreeConditionCommonBase                               
{                                                                                                               
	GENERATED_BODY()                                                                                            
	
	using FInstanceDataType = FOnsetStateTreeTargetConditionInstanceData;
	
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}
	
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override                              
	{                                                                                                           
		return FOnsetStateTreeTaskBase::GetTarget(Context) == nullptr;                                          
	}                                                                                                           
};       