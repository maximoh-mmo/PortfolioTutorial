#include "StateTree/Tasks/OnsetStateTreeTask.h"
#include "GameplayEffect.h"
#include "StateTreeExecutionContext.h"
#include "AI/OnsetAIController.h"
#include "Enemy/OnsetEnemy.h"
#include "GAS/OnsetMovementAttributeSet.h"

#include "Navigation/PathFollowingComponent.h"
#include "Core/OnsetBaseCharacter.h"
#include "Player/OnsetPlayerAIController.h"
#include "Core/TargetingComponent.h"

AOnsetAIController* FOnsetStateTreeTask::GetController(const FStateTreeExecutionContext& Context)
{
	AOnsetAIController* AIController = Cast<AOnsetAIController>(Context.GetOwner());
	if (!AIController) UE_LOG(LogTemp, Warning, TEXT("GetController: Context owner is not an AOnsetAIController"));
	return AIController;
}

AOnsetPlayerAIController* FOnsetStateTreeTask::GetPlayerController(const FStateTreeExecutionContext& Context)
{
	AOnsetPlayerAIController* AIController = Cast<AOnsetPlayerAIController>(Context.GetOwner());
	if (!AIController) UE_LOG(LogTemp, Warning, TEXT("GetController: Context owner is not an AOnsetPlayerAIController"));
	return AIController;
}

UTargetingComponent* FOnsetStateTreeTask::GetTargetingComponent(const FStateTreeExecutionContext& Context)
{
	AAIController* Controller = Cast<AAIController>(Context.GetOwner());                                                                                                               
	if (!Controller) return nullptr;                                                                                                                                                   
                                                                                                                                                                                            
	if (const AOnsetAIController* OnsetAIController = Cast<AOnsetAIController>(Controller))                                                                                                    
		return OnsetAIController->TargetingComponent;                                                                                                                                          
                                                                                                                                                                                            
	if (APawn* Pawn = Controller->GetPawn())                                                                                                                                           
		return Pawn->FindComponentByClass<UTargetingComponent>();                                                                                                                      
                                                                                                                                                                                            
	return nullptr;      
}

AActor* FOnsetStateTreeTask::GetTarget(const FStateTreeExecutionContext& Context)
{
	const UTargetingComponent* TargetingComponent = GetTargetingComponent(Context);
	if (!TargetingComponent) return nullptr;
	return TargetingComponent->GetTarget();
}

void FOnsetStateTreeTask::SetTarget(const FStateTreeExecutionContext& Context, AActor* NewTarget)
{	
	if (GetTargetingComponent(Context) == nullptr) return;
	GetTargetingComponent(Context)->SetTarget(NewTarget);
}

bool FOnsetStateTreeTask::HasMoveCompleted(const FStateTreeExecutionContext& Context)
{
	const AOnsetAIController* AIController = GetController(Context);                                                           
	if (!AIController) return false;
	const UPathFollowingComponent* PathFollowingComponent = AIController->GetPathFollowingComponent();                                            
	return PathFollowingComponent && PathFollowingComponent->DidMoveReachGoal();
}

UPathFollowingComponent* FOnsetStateTreeTask::GetPathFollowingComponent(const FStateTreeExecutionContext& Context)
{
	const AOnsetAIController* AIController = GetController(Context);
	if (!AIController) return nullptr;
	if (UPathFollowingComponent* PathFollowingComponent = AIController->GetPathFollowingComponent()) return PathFollowingComponent;
	UE_LOG(LogTemp, Warning, TEXT("GetPathFollowingComponent: No pathing component"));
	return nullptr;
}

FActiveGameplayEffectHandle FOnsetStateTreeTask::ApplyMovementSpeedModifier(const AOnsetBaseCharacter* Self,
	const float Magnitude)
{
	if (!Self || !Self->AbilitySystemComponent)	return FActiveGameplayEffectHandle();
		
	UGameplayEffect* SpeedGE = NewObject<UGameplayEffect>(GetTransientPackageAsObject(),FName("DynamicSpeedModifier"));
	SpeedGE->DurationPolicy = EGameplayEffectDurationType::Infinite;
	if (SpeedGE->Modifiers.Num()==0)
	{
		FGameplayModifierInfo ModifierInfo;
		ModifierInfo.Attribute = UOnsetMovementAttributeSet::GetMovementSpeedAttribute();
		ModifierInfo.ModifierOp = EGameplayModOp::MultiplyCompound;
		ModifierInfo.ModifierMagnitude = FScalableFloat(Magnitude);
		SpeedGE->Modifiers.Add(ModifierInfo);
	}
	else
	{
		SpeedGE->Modifiers[0].Attribute = UOnsetMovementAttributeSet::GetMovementSpeedAttribute();
		SpeedGE->Modifiers[0].ModifierOp = EGameplayModOp::MultiplyCompound;
		SpeedGE->Modifiers[0].ModifierMagnitude = FScalableFloat(Magnitude);
	}
		
	return Self->AbilitySystemComponent->ApplyGameplayEffectToSelf(
		SpeedGE,1.0f,Self->AbilitySystemComponent->MakeEffectContext());
}
