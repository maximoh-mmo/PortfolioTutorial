// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OnsetStateTreeHasTargetCondition.h"
#include "StateTreeConditionBase.h"
#include "OnsetStateTreeHasNoTargetCondition.generated.h"


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