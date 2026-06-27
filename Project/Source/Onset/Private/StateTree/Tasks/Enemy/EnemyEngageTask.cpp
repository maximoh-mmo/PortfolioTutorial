#include "StateTree/Tasks/Enemy/EnemyEngageTask.h"

#include "AbilitySystemComponent.h"
#include "NavigationSystem.h"
#include "StateTreeExecutionContext.h"
#include "AI/OnsetAIController.h"
#include "Combat/OnsetGA_BasicAttack.h"
#include "Enemy/OnsetEnemy.h"
#include "GAS/OnsetGameplayTags.h"
#include "Core/OnsetBaseCharacter.h"
#include "Enemy/Profile/AIProfile.h"
#include "Subsystem/OnsetThreatSubsystem.h"

DEFINE_LOG_CATEGORY(LogEnemyEngageTask);

EStateTreeRunStatus FEnemyEngageTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	AOnsetAIController* AIController = GetController(Context);
	if (!AIController)
	{
		UE_LOG(LogEnemyEngageTask, Warning, TEXT("EnterState: AIController is null. Returning Failed."));
		return EStateTreeRunStatus::Failed;
	}
	AOnsetEnemy* SelfEnemy = GetSelfPawn<AOnsetEnemy>(Context);
	if (!SelfEnemy)
	{
		UE_LOG(LogEnemyEngageTask, Warning, TEXT("EnterState: SelfEnemy is null. Returning Failed."));
		return EStateTreeRunStatus::Failed;
	}

	FInstanceDataType& Inst = Context.GetInstanceData(*this);

	if (const UAIProfile* Profile = AIController->GetAIProfile())
	{
		Inst.AttackRange = Profile->AttackRange;
		Inst.ChaseRange = Profile->ChaseRange;
	}
	
	const float Stagger = FMath::FRand() * 2.0f;
	Inst.NextPositionReevaluateTime = Stagger;
	Inst.NextTargetReevaluateTime = Stagger;
	Inst.TimeInState = 0.0f;
	Inst.TimeWithoutTarget = 0.0f;

	UOnsetThreatSubsystem* Subsystem = GetThreatSubsystem(Context);
	if (Subsystem)
	{
		AOnsetBaseCharacter* Best = Subsystem->GetBestTarget(SelfEnemy, Inst.AttackRange, Inst.ChaseRange);
		if (Best)
		{
			SetTarget(Context, Best);
			Subsystem->RegisterEngaged(Best, SelfEnemy);
			AIController->SetFocus(Best);
			Inst.CurrentTarget = Best;
			Inst.LastTargetLocation = Best->GetActorLocation();
		}
	}

	Inst.CurrentOffsetPosition = ComputeOffsetPosition(Context, Inst, SelfEnemy);
	AIController->MoveToLocation(Inst.CurrentOffsetPosition, Inst.AcceptanceRadius);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FEnemyEngageTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Inst = Context.GetInstanceData(*this);
	Inst.TimeInState += DeltaTime;

	AOnsetAIController* AIController = GetController(Context);
	if (!AIController)
	{
		UE_LOG(LogEnemyEngageTask, Warning, TEXT("Tick: AIController is null. Returning Failed."))
		return EStateTreeRunStatus::Failed;
	}
	AOnsetEnemy* SelfEnemy = GetSelfPawn<AOnsetEnemy>(Context);
	if (!SelfEnemy)
	{
		UE_LOG(LogEnemyEngageTask, Warning, TEXT("Tick: SelfEnemy is null. Returning Failed."))
		return EStateTreeRunStatus::Failed;
	}
	
	UOnsetThreatSubsystem* Subsystem = GetThreatSubsystem(Context);

	AActor* CurrentContextTarget = GetTarget(Context);

	// Track sustained absence of target before giving up
	if (!CurrentContextTarget && !Inst.CurrentTarget)
	{
		Inst.TimeWithoutTarget += DeltaTime;
		if (Inst.TimeWithoutTarget >= 2.0f)
		{
			return EStateTreeRunStatus::Succeeded;
		}
	}
	else
	{
		Inst.TimeWithoutTarget = 0.0f;
	}

	// Target re-evaluation
	if (Subsystem && Inst.TimeInState >= Inst.NextTargetReevaluateTime)
	{
		Inst.NextTargetReevaluateTime = Inst.TimeInState + Inst.TargetReevaluateInterval;

		AOnsetBaseCharacter* Best = Subsystem->GetBestTarget(SelfEnemy, Inst.AttackRange, Inst.ChaseRange);
		AOnsetBaseCharacter* CurrentTargetPtr = Inst.CurrentTarget;

		if (Best && Best != CurrentTargetPtr)
		{
			Subsystem->SwitchTarget(SelfEnemy, Best);
			SetTarget(Context, Best);
			AIController->SetFocus(Best);
			Inst.CurrentTarget = Best;
			Inst.LastTargetLocation = Best->GetActorLocation();
			Inst.NextPositionReevaluateTime = 0.0f;
		}
		else if (!Best && CurrentTargetPtr)
		{
			Inst.CurrentTarget = nullptr; // Clear current target if no best target is found
			SetTarget(Context, nullptr);
			AIController->ClearFocus(EAIFocusPriority::Gameplay);
		}
	}

	AActor* Target = GetTarget(Context);
	if (!Target)
	{
		return EStateTreeRunStatus::Running;
	}

	const FVector TargetLoc = Target->GetActorLocation();
	const float DistSq = FVector::DistSquared(SelfEnemy->GetActorLocation(), TargetLoc);

	// Position re-evaluation
	bool bShouldRepath = Inst.TimeInState >= Inst.NextPositionReevaluateTime;
	if (!bShouldRepath)
	{
		const float MoveDistSq = FVector::DistSquared(Inst.LastTargetLocation, TargetLoc);
		if (MoveDistSq > FMath::Square(Inst.MoveThreshold))
			bShouldRepath = true;
	}

	if (bShouldRepath)
	{
		Inst.NextPositionReevaluateTime = Inst.TimeInState + Inst.PositionReevaluateInterval;
		Inst.LastTargetLocation = TargetLoc;

		const FVector NewOffset = ComputeOffsetPosition(Context, Inst, SelfEnemy);
		if (!NewOffset.Equals(Inst.CurrentOffsetPosition, 50.0f))
		{
			Inst.CurrentOffsetPosition = NewOffset;
			AIController->MoveToLocation(Inst.CurrentOffsetPosition, Inst.AcceptanceRadius);
		}
	}

	if (DistSq > FMath::Square(Inst.AttackRange))
	{
		return EStateTreeRunStatus::Running;
	}

	AIController->StopMovement();
	AIController->SetFocus(Target);

	if (SelfEnemy->AbilitySystemComponent &&
		!SelfEnemy->AbilitySystemComponent->HasMatchingGameplayTag(TAG_Cooldown_BasicAttack))
	{
		SelfEnemy->AbilitySystemComponent->TryActivateAbilityByClass(Inst.AbilityClass);
	}

	return EStateTreeRunStatus::Running;
}

void FEnemyEngageTask::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	AOnsetAIController* AIController = GetController(Context);
	if (AIController) AIController->StopMovement();
}

FVector FEnemyEngageTask::ComputeOffsetPosition(const FStateTreeExecutionContext& Context,
	const FInstanceDataType& Inst, AOnsetEnemy* SelfEnemy) const
{
	AActor* Target = GetTarget(Context);
	if (!Target) return FVector::ZeroVector;

	AOnsetBaseCharacter* TargetChar = Cast<AOnsetBaseCharacter>(Target);
	if (!TargetChar) return Target->GetActorLocation() + FVector(Inst.AttackRange, 0, 0);

	UOnsetThreatSubsystem* Subsystem = GetThreatSubsystem(Context);
	if (!Subsystem) return Target->GetActorLocation();

	const float Dist = FVector::Dist(SelfEnemy->GetActorLocation(), Target->GetActorLocation());
	const float Radius = (Dist > Inst.AttackRange) ? Inst.SpreadRadius : Inst.AttackRange;

	const int32 Count = Subsystem->GetEngagedCount(TargetChar);
	const int32 Rank = Subsystem->GetEngagedIndex(SelfEnemy, TargetChar);

	FVector Offset = GetThreatAngularOffset(Count, Rank, Radius);

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(SelfEnemy->GetWorld());
	FNavLocation Projected;
	if (NavSys && NavSys->ProjectPointToNavigation(Target->GetActorLocation() + Offset, Projected, FVector(Radius * 0.5f)))
		return Projected.Location;

	return Target->GetActorLocation() + Offset;
}