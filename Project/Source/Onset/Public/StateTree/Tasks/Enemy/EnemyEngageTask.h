#pragma once

#include "CoreMinimal.h"
#include "StateTree/Tasks/OnsetStateTreeTask.h"
#include "EnemyEngageTask.generated.h"

class UGameplayAbility;

USTRUCT()
struct FOnsetStateTreeEngageInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Engage")
	TSubclassOf<UGameplayAbility> AbilityClass;

	UPROPERTY(EditAnywhere, Category = "Engage")
	float AttackRange = 250.0f;

	UPROPERTY(EditAnywhere, Category = "Engage")
	float ChaseRange = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "Engage")
	float SpreadRadius = 280.0f;

	UPROPERTY(EditAnywhere, Category = "Engage")
	float AcceptanceRadius = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Engage")
	float PositionReevaluateInterval = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Engage")
	float TargetReevaluateInterval = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Engage")
	float MoveThreshold = 200.0f;

	float TimeInState = 0.0f;
	float NextPositionReevaluateTime = 0.0f;
	float NextTargetReevaluateTime = 0.0f;
	FVector CurrentOffsetPosition = FVector::ZeroVector;
	FVector LastTargetLocation = FVector::ZeroVector;
	TWeakObjectPtr<AOnsetBaseCharacter> CurrentTarget;
};

USTRUCT()
struct FEnemyEngageTask : public FOnsetStateTreeTask
{
	GENERATED_BODY()

	using FInstanceDataType = FOnsetStateTreeEngageInstanceData;

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

	virtual void ExitState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

private:
	FVector ComputeOffsetPosition(const FStateTreeExecutionContext& Context,
		const FInstanceDataType& Inst, AOnsetEnemy* SelfEnemy) const;
};
