// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "StateTreeTaskBase.h"
#include "AI/OnsetAIController.h"
#include "Player/OnsetPlayerAIController.h"
#include "OnsetStateTreeTask.generated.h"

class UPathFollowingComponent;
class AOnsetBaseCharacter;
class AOnsetAIController;
class AOnsetEnemy;

USTRUCT()
struct FOnsetStateTreeTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()                                                                                            
                                                                                                                     
	static AOnsetAIController* GetController(const FStateTreeExecutionContext& Context);
	static AOnsetPlayerAIController* GetPlayerController(const FStateTreeExecutionContext& Context);
	
	static UTargetingComponent* GetTargetingComponent(const FStateTreeExecutionContext& Context);

	static AActor* GetTarget(const FStateTreeExecutionContext& Context);
	static void SetTarget(const FStateTreeExecutionContext& Context, AActor* NewTarget);

	static bool HasMoveCompleted(const FStateTreeExecutionContext& Context);
	
	static UPathFollowingComponent* GetPathFollowingComponent(const FStateTreeExecutionContext& Context);
	
	static FActiveGameplayEffectHandle ApplyMovementSpeedModifier(const AOnsetBaseCharacter* Self, const float Magnitude);
	
	template<typename T>
	static T* GetSelfPawn(const FStateTreeExecutionContext& Context)
	{
		const AOnsetAIController* Controller = GetController(Context);
		if (!Controller) return nullptr;
		T* Result = Cast<T>(Controller->GetPawn());
		if (!Result)
			UE_LOG(LogStateTree, Warning, TEXT("GetSelfPawn: Failed to get Pawn of type %s"), *GetNameSafe(T::StaticClass()));
		return Result;
	}

};
