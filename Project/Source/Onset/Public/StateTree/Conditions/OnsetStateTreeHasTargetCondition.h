#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "StateTree/Tasks/OnsetStateTreeTask.h"
#include "OnsetStateTreeHasTargetCondition.generated.h"

USTRUCT()                                                                                                       
struct FOnsetStateTreeTargetConditionInstanceData                                                            
{                                                                                                               
	GENERATED_BODY()                                                                                            
};     

USTRUCT()
struct FOnsetStateTreeHasTargetCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()
	
	using FInstanceDataType = FOnsetStateTreeTargetConditionInstanceData;
	
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}
	
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override
	{
		return FOnsetStateTreeTask::GetTarget(Context) != nullptr;
	}
};
