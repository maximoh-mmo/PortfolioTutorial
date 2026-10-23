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
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
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
	Inst.CurrentTarget = nullptr;

	UOnsetThreatSubsystem* Subsystem = GetThreatSubsystem(Context);
	AOnsetBaseCharacter* Best = Subsystem ? Subsystem->GetBestTarget(SelfEnemy, Inst.AttackRange, Inst.ChaseRange) : nullptr;
	if (!Best)
	{
		// Fall back to perceived target if valid and alive
		if (AOnsetBaseCharacter* PerceivedChar = Cast<AOnsetBaseCharacter>(GetTarget(Context)))
		{
			if (PerceivedChar->IsAlive())
			{
				Best = PerceivedChar;
				if (Subsystem)
				{
					Subsystem->AddThreat(Best, SelfEnemy, 1.0f);
				}
			}
		}
	}

	if (!Best)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	SetTarget(Context, Best);
	if (Subsystem)
	{
		Subsystem->RegisterEngaged(Best, SelfEnemy);
	}
	AIController->SetFocus(Best);
	Inst.CurrentTarget = Best;
	Inst.LastTargetLocation = Best->GetActorLocation();

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

	// Immediately invalidate CurrentTarget if it died or was destroyed
	if (Inst.CurrentTarget && (!IsValid(Inst.CurrentTarget) || !Inst.CurrentTarget->IsAlive()))
	{
		if (Subsystem)
		{
			Subsystem->RemovePlayer(Inst.CurrentTarget);
		}
		Inst.CurrentTarget = nullptr;
		SetTarget(Context, nullptr);
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		Inst.NextTargetReevaluateTime = 0.0f;
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

	AActor* CurrentContextTarget = GetTarget(Context);

	// Track absence of target or exit immediately if threat table is empty
	if (!CurrentContextTarget && !Inst.CurrentTarget)
	{
		if (!Subsystem || Subsystem->GetTargetCount(SelfEnemy) == 0)
		{
			return EStateTreeRunStatus::Succeeded;
		}
		Inst.TimeWithoutTarget += DeltaTime;
		if (Inst.TimeWithoutTarget >= 2.0f)
		{
			return EStateTreeRunStatus::Succeeded;
		}
		return EStateTreeRunStatus::Running;
	}
	else
	{
		Inst.TimeWithoutTarget = 0.0f;
	}

	AActor* Target = CurrentContextTarget ? CurrentContextTarget : Inst.CurrentTarget.Get();
	if (!Target)
	{
		return EStateTreeRunStatus::Running;
	}

	const FVector SelfLoc = SelfEnemy->GetActorLocation();
	const FVector TargetLoc = Target->GetActorLocation();
	const FVector HomeLoc = SelfEnemy->HomeTransform.GetLocation();

	// Leash check:
	// 1. Territory leash: distance from SelfEnemy (or Target) to HomeTransform exceeds ChaseRange
	// 2. Pursuit leash: distance from SelfEnemy to Target exceeds max(ChaseRange, SightRange * 1.25)
	if (Inst.ChaseRange > 0.0f)
	{
		const float LeashDistSq = FMath::Square(Inst.ChaseRange);
		const float MaxPursuitDistSq = FMath::Square(FMath::Max(Inst.ChaseRange, AIController->CachedSightRange * 1.25f));
		const float DistFromHomeSq = FVector::DistSquared2D(SelfLoc, HomeLoc);
		const float DistToTargetSq = FVector::DistSquared2D(SelfLoc, TargetLoc);

		if (DistFromHomeSq > LeashDistSq || DistToTargetSq > MaxPursuitDistSq)
		{
			if (Subsystem)
			{
				Subsystem->RemoveEnemy(SelfEnemy);
			}
			Inst.CurrentTarget = nullptr;
			SetTarget(Context, nullptr);
			AIController->ClearFocus(EAIFocusPriority::Gameplay);
			if (UAIPerceptionComponent* Perc = AIController->GetPerceptionComponent())
			{
				Perc->ForgetAll();
			}
			AIController->bIsReturningHome = true;
			return EStateTreeRunStatus::Succeeded;
		}
	}

	const float DistSq = FVector::DistSquared(SelfLoc, TargetLoc);

	// Position re-evaluation
	bool bShouldRepath = Inst.TimeInState >= Inst.NextPositionReevaluateTime;
	if (!bShouldRepath)
	{
		const float MoveDistSq = FVector::DistSquared(Inst.LastTargetLocation, TargetLoc);
		if (MoveDistSq > FMath::Square(Inst.MoveThreshold))
			bShouldRepath = true;
	}

	// If outside attack range and pathing has already gone idle, force a repath into attack range
	if (!bShouldRepath && DistSq > FMath::Square(Inst.AttackRange))
	{
		if (const UPathFollowingComponent* PF = GetPathFollowingComponent(Context))
		{
			if (PF->GetStatus() == EPathFollowingStatus::Idle)
			{
				bShouldRepath = true;
			}
		}
	}

	if (bShouldRepath)
	{
		Inst.NextPositionReevaluateTime = Inst.TimeInState + Inst.PositionReevaluateInterval;
		Inst.LastTargetLocation = TargetLoc;

		const FVector NewOffset = ComputeOffsetPosition(Context, Inst, SelfEnemy);
		if (!NewOffset.Equals(Inst.CurrentOffsetPosition, 25.0f) || DistSq > FMath::Square(Inst.AttackRange))
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
	if (UOnsetThreatSubsystem* Subsystem = GetThreatSubsystem(Context))
	{
		if (AOnsetEnemy* SelfEnemy = GetSelfPawn<AOnsetEnemy>(Context))
		{
			Subsystem->UnregisterEngaged(SelfEnemy);
		}
	}
	AOnsetAIController* AIController = GetController(Context);
	if (AIController)
	{
		AIController->StopMovement();
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
}

FVector FEnemyEngageTask::ComputeOffsetPosition(const FStateTreeExecutionContext& Context,
	const FInstanceDataType& Inst, AOnsetEnemy* SelfEnemy)
{
	AActor* Target = GetTarget(Context);
	if (!Target) return FVector::ZeroVector;

	// Ensure the target ring radius plus MoveToLocation's AcceptanceRadius stays strictly inside AttackRange
	const float CombatRadius = FMath::Max(60.0f, Inst.AttackRange - Inst.AcceptanceRadius - 20.0f);

	AOnsetBaseCharacter* TargetChar = Cast<AOnsetBaseCharacter>(Target);
	if (!TargetChar) return Target->GetActorLocation() + FVector(CombatRadius, 0, 0);

	UOnsetThreatSubsystem* Subsystem = GetThreatSubsystem(Context);
	if (!Subsystem) return Target->GetActorLocation();

	const int32 Count = Subsystem->GetEngagedCount(TargetChar);
	const int32 Rank = Subsystem->GetEngagedIndex(SelfEnemy, TargetChar);

	FVector Offset;
	if (Count <= 1)
	{
		// Single attacker: approach directly along the line from Target to SelfEnemy
		FVector ApproachDir = (SelfEnemy->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
		if (ApproachDir.IsNearlyZero())
		{
			ApproachDir = FVector::ForwardVector;
		}
		Offset = ApproachDir * CombatRadius;
	}
	else
	{
		Offset = GetThreatAngularOffset(Count, Rank, CombatRadius);
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(SelfEnemy->GetWorld());
	FNavLocation Projected;
	if (NavSys && NavSys->ProjectPointToNavigation(Target->GetActorLocation() + Offset, Projected, FVector(CombatRadius)))
		return Projected.Location;

	return Target->GetActorLocation() + Offset;
}
