#include "AI/Tasks/OnsetStateTreeTaskBase.h"
#include "GameplayEffect.h"
#include "StateTreeExecutionContext.h"
#include "AI/OnsetAIController.h"
#include "Enemy/OnsetEnemy.h"
#include "GAS/OnsetMovementAttributeSet.h"
#include "Navigation/PathFollowingComponent.h"
#include "Player/OnsetBaseCharacter.h"
#include "Player/TargetingComponent.h"

AOnsetAIController* FOnsetStateTreeTaskBase::GetController(const FStateTreeExecutionContext& Context)
{
	AOnsetAIController* AIController = Cast<AOnsetAIController>(Context.GetOwner());
	if (!AIController) UE_LOG(LogTemp, Warning, TEXT("GetController: Context owner is not an AOnsetAIController"));
	return AIController;
}

AOnsetBaseCharacter* FOnsetStateTreeTaskBase::GetSelfBaseCharacter(const FStateTreeExecutionContext& Context)
{
	const AOnsetAIController* AIController = GetController(Context);                                                                        
	if (!AIController) return nullptr;                                                                               
	AOnsetBaseCharacter* BaseCharacter = Cast<AOnsetBaseCharacter>(AIController->GetPawn());                                                 
	if (!BaseCharacter) UE_LOG(LogTemp, Warning, TEXT("GetSelfBaseCharacter: Pawn is not an AOnsetBaseCharacter"));                 
	return BaseCharacter;
}

AOnsetEnemy* FOnsetStateTreeTaskBase::GetSelfEnemyCharacter(const FStateTreeExecutionContext& Context)
{
	const AOnsetAIController* AIController = GetController(Context);
	if (!AIController) return nullptr;
	AOnsetEnemy* EnemyCharacter = Cast<AOnsetEnemy>(AIController->GetPawn());
	if (!EnemyCharacter) UE_LOG(LogTemp, Warning, TEXT("GetSelfEnemyCharacter: Pawn is not an AOnsetEnemy"))
	return EnemyCharacter;
}

AActor* FOnsetStateTreeTaskBase::GetTarget(const FStateTreeExecutionContext& Context)
{
	const AOnsetAIController* AIController = GetController(Context);
	if (!AIController) return nullptr;       
	if (AIController->TargetingComponent) return AIController->TargetingComponent->GetTarget();
	UE_LOG(LogTemp, Warning, TEXT("GetTarget: No targeting component"));
	return nullptr;
}

bool FOnsetStateTreeTaskBase::HasMoveCompleted(const FStateTreeExecutionContext& Context)
{
	const AOnsetAIController* AIController = GetController(Context);                                                           
	if (!AIController) return false;
	const UPathFollowingComponent* PathFollowingComponent = AIController->GetPathFollowingComponent();                                            
	return PathFollowingComponent && PathFollowingComponent->DidMoveReachGoal();
}

UPathFollowingComponent* FOnsetStateTreeTaskBase::GetPathFollowingComponent(const FStateTreeExecutionContext& Context)
{
	const AOnsetAIController* AIController = GetController(Context);
	if (!AIController) return nullptr;
	if (UPathFollowingComponent* PathFollowingComponent = AIController->GetPathFollowingComponent()) return PathFollowingComponent;
	UE_LOG(LogTemp, Warning, TEXT("GetPathFollowingComponent: No pathing component"));
	return nullptr;
}

struct FActiveGameplayEffectHandle FOnsetStateTreeTaskBase::ApplyMovementSpeedModifier(const AOnsetBaseCharacter* Self,
	const float Magnitude)
{
	if (!Self || !Self->AbilitySystemComponent) return FActiveGameplayEffectHandle();
		
	UGameplayEffect* SpeedGE = NewObject<UGameplayEffect>(GetTransientPackageAsObject(),FName("DynamicSpeedModifier"));
	SpeedGE->DurationPolicy = EGameplayEffectDurationType::Infinite;
	FGameplayModifierInfo ModifierInfo;
	ModifierInfo.Attribute = UOnsetMovementAttributeSet::GetMovementSpeedAttribute();
	ModifierInfo.ModifierOp = EGameplayModOp::MultiplyCompound;
	ModifierInfo.ModifierMagnitude = FScalableFloat(Magnitude);
	SpeedGE->Modifiers.Add(ModifierInfo);
		
	return Self->AbilitySystemComponent->ApplyGameplayEffectToSelf(
		SpeedGE,1.0f,Self->AbilitySystemComponent->MakeEffectContext());
}
