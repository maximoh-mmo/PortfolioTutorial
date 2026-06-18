
#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "StateTreeTaskBase.h"
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

	static AOnsetBaseCharacter* GetSelfBaseCharacter(const FStateTreeExecutionContext& Context);
	
	static AOnsetEnemy* GetSelfEnemyCharacter(const FStateTreeExecutionContext& Context);
	static UTargetingComponent* GetTargetingComponent(const FStateTreeExecutionContext& Context);

	static AActor* GetTarget(const FStateTreeExecutionContext& Context);
	static void SetTarget(const FStateTreeExecutionContext& Context, AActor* NewTarget);

	static bool HasMoveCompleted(const FStateTreeExecutionContext& Context);
	
	static UPathFollowingComponent* GetPathFollowingComponent(const FStateTreeExecutionContext& Context);
	
	static FActiveGameplayEffectHandle ApplyMovementSpeedModifier(const AOnsetBaseCharacter* Self, const float Magnitude);

};
