
#pragma once

#include "CoreMinimal.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeTaskBase.h"
#include "AI/OnsetAIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "Player/OnsetBaseCharacter.h"
#include "Player/TargetingComponent.h"
#include "OnsetStateTreeTaskBase.generated.h"

USTRUCT()
struct FOnsetStateTreeTaskBase : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()                                                                                            
                                                                                                                     
	static AOnsetAIController* GetController(const FStateTreeExecutionContext& Context)                                      
	{
		AOnsetAIController* AIController = Cast<AOnsetAIController>(Context.GetOwner());
		if (!AIController) UE_LOG(LogTemp, Warning, TEXT("GetController: Context owner is not an AOnsetAIController"));
		return AIController;                                                                                             
	}                                                                                                           
                                                                                                                     
    static AOnsetBaseCharacter* GetSelfBaseCharacter(const FStateTreeExecutionContext& Context)
	{
		const AOnsetAIController* AIController = GetController(Context);                                                                        
		if (!AIController) return nullptr;                                                                               
		AOnsetBaseCharacter* BaseCharacter = Cast<AOnsetBaseCharacter>(AIController->GetPawn());                                                 
		if (!BaseCharacter) UE_LOG(LogTemp, Warning, TEXT("GetSelfBaseCharacter: Pawn is not an AOnsetBaseCharacter"));                 
		return BaseCharacter;
	}                                                                                                           
	
	static AActor* GetTarget(const FStateTreeExecutionContext& Context)
	{
		AOnsetAIController* AIController = GetController(Context);
		if (!AIController) return nullptr;       
		if (AIController->TargetingComponent) return AIController->TargetingComponent->GetTarget();
		UE_LOG(LogTemp, Warning, TEXT("GetTarget: No targeting component"));
		return nullptr;
	}
	
	static bool HasMoveCompleted(const FStateTreeExecutionContext& Context)                                         
	{
		const AOnsetAIController* AIController = GetController(Context);                                                           
		if (!AIController) return false;
		const UPathFollowingComponent* PathFollowingComponent = AIController->GetPathFollowingComponent();                                            
		return PathFollowingComponent && PathFollowingComponent->DidMoveReachGoal();                                                                      
	}     
	
	static UPathFollowingComponent* GetPathFollowingComponent(const FStateTreeExecutionContext& Context)
	{
		AOnsetAIController* AIController = GetController(Context);
		if (!AIController) return nullptr;
		if (UPathFollowingComponent* PathFollowingComponent = AIController->GetPathFollowingComponent()) return PathFollowingComponent;
		UE_LOG(LogTemp, Warning, TEXT("GetPathFollowingComponent: No pathing component"));
		return nullptr;
	}
};
