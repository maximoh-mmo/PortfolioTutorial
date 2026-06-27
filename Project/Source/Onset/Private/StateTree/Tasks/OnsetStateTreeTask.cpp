#include "StateTree/Tasks/OnsetStateTreeTask.h"
#include "GameplayEffect.h"
#include "StateTreeExecutionContext.h"
#include "AI/OnsetAIController.h"
#include "GAS/OnsetMovementAttributeSet.h"
#include "GAS/OnsetMovementSpeedModifierEffect.h"
#include "Subsystem/OnsetThreatSubsystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Core/OnsetBaseCharacter.h"
#include "Player/OnsetPlayerAIController.h"
#include "Core/TargetingComponent.h"
#include "Engine/World.h"

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

UOnsetThreatSubsystem* FOnsetStateTreeTask::GetThreatSubsystem(const FStateTreeExecutionContext& Context)
{
	AAIController* Controller = Cast<AAIController>(Context.GetOwner());
	if (!Controller || !Controller->GetWorld()) return nullptr;
	return Controller->GetWorld()->GetSubsystem<UOnsetThreatSubsystem>();
}

FVector FOnsetStateTreeTask::GetThreatAngularOffset(int32 Count, int32 Rank, float Radius)
{
	if (Count <= 0 || Rank < 0 || Rank >= Count) return FVector::ZeroVector;
	float Angle = Rank / static_cast<float>(Count) * 360.0f;
	float Rad = FMath::DegreesToRadians(Angle);
	return FVector(FMath::Cos(Rad) * Radius, FMath::Sin(Rad) * Radius, 0);	
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
	if (!Self || !Self->AbilitySystemComponent) return FActiveGameplayEffectHandle();

	UAbilitySystemComponent* ASC = Self->AbilitySystemComponent;
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
		UOnsetMovementSpeedModifierEffect::StaticClass(), 1.0f, ASC->MakeEffectContext());
	if (!SpecHandle.IsValid()) return FActiveGameplayEffectHandle();

	FGameplayEffectSpec* Spec = SpecHandle.Data.Get();
	if (Spec)
	{
		Spec->SetSetByCallerMagnitude(FName("MoveSpeedMod"), Magnitude);
	}

	return ASC->ApplyGameplayEffectSpecToSelf(*Spec);
}
