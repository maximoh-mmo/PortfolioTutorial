// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "StateTreeTaskBase.h"
#include "AI/OnsetAIController.h"
#include "Player/OnsetPlayerAIController.h"
#include "OnsetStateTreeTask.generated.h"

class UOnsetThreatSubsystem;
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
	static UOnsetThreatSubsystem* GetThreatSubsystem(const FStateTreeExecutionContext& Context);
	static FVector GetThreatAngularOffset(int32 Count, int32 Rank, float Radius);
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
