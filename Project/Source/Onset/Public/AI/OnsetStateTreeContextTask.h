// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "OnsetStateTreeContext.h"
#include "OnsetStateTreeContextTask.generated.h"

/** Global StateTree task that keeps FOnsetStateTreeContextData up to date. */
USTRUCT()
struct FOnsetStateTreeContextTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
	
	using FInstanceDataType = FOnsetStateTreeContextData;
	
	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& TransitionResult) const override;
	
	virtual EStateTreeRunStatus Tick(
		FStateTreeExecutionContext& Context
		, const float DeltaTime) const override;
	
};