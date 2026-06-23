// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTree/Tasks/OnsetStateTreeTask.h"
#include "EnemyAttackTask.generated.h"

class UGameplayAbility;

/** Runtime data for the enemy attack task. */
USTRUCT()
struct FOnsetStateTreeAttackTaskInstanceData
{
	GENERATED_BODY()

	/** Gameplay ability class to activate when attacking. */
	UPROPERTY(EditAnywhere, Category = "Attack")
	TSubclassOf<UGameplayAbility> AbilityClass;

};


USTRUCT()
struct FEnemyAttackTask : public FOnsetStateTreeTask
{
	GENERATED_BODY()
	
	using FInstanceDataType = FOnsetStateTreeAttackTaskInstanceData; 
	
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}
	
	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(
		FStateTreeExecutionContext& Context,
		const float DeltaTime) const override;
};
