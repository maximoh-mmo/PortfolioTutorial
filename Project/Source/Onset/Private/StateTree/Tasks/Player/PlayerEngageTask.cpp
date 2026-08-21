#include "StateTree/Tasks/Player/PlayerEngageTask.h"

#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "StateTreeExecutionContext.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Combat/OnsetGA_Generic.h"
#include "Combat/OnsetAbilityLibrary.h"
#include "Data/OnsetAbilityTypes.h"
#include "GAS/OnsetGameplayTags.h"
#include "Player/OnsetPlayerCharacter.h"
#include "Core/TargetingComponent.h"

namespace
{
	/** Enemies among OverlapActors within Radius of Center. */
	int32 CountEnemiesWithin(const TArray<AActor*>& OverlapActors, const FVector& Center, float Radius)
	{
		const float RadiusSq = Radius * Radius;
		int32 Count = 0;
		for (const AActor* Actor : OverlapActors)
		{
			if (Actor && FVector::DistSquared(Center, Actor->GetActorLocation()) <= RadiusSq)
			{
				++Count;
			}
		}
		return Count;
	}

	/**
	 * Enemies within Radius of Origin whose direction falls inside the cone:
	 * half-angle ConeHalfAngleDegrees around AimDir (the cast direction toward the
	 * primary target), mirroring GA_Generic's cone resolution.
	 */
	int32 CountConeHits(const TArray<AActor*>& OverlapActors, const FVector& Origin,
						const FVector& AimDir, float Radius, float ConeHalfAngleDegrees)
	{
		const float RadiusSq = Radius * Radius;
		const float MinDot = FMath::Cos(FMath::DegreesToRadians(ConeHalfAngleDegrees));
		int32 Count = 0;
		for (const AActor* Actor : OverlapActors)
		{
			if (!Actor) continue;
			const FVector ToActor = Actor->GetActorLocation() - Origin;
			if (ToActor.SizeSquared() > RadiusSq) continue;
			if (FVector::DotProduct(ToActor.GetSafeNormal(), AimDir) >= MinDot)
			{
				++Count;
			}
		}
		return Count;
	}

	/** True when the ability has an enemy-facing periodic damage effect (a refreshable DoT). */
	bool HasHostilePeriodicDamage(const FOnsetAbilityDefinition& Definition)
	{
		for (const FOnsetAbilityEffect& Effect : Definition.Effects)
		{
			if (Effect.Type == EOnsetAbilityEffectType::Damage
				&& Effect.Period > 0.0f
				&& !Effect.bFriendly)
			{
				return true;
			}
		}
		return false;
	}
}

/**
	- Gather cooldown-ready attack abilities from the ASC
	- Score each by expected total damage: GetComparisonDamage (per-target) x hit count,
	  where hit count comes from one shared overlap query shaped per AbilityType
	(AoE/PBAoE radius, Cone half-angle; SingleTarget/Self = 1). Refreshable DoTs whose
	RefreshTag is already active on the primary target are dropped (DoTs refresh, not stack).
	- Pick highest score; ties go to the LONGEST cooldown so expensive casts go on cooldown
	before fast fillers.
	- Non-generic attacks (GA_BasicAttack) are zero-score fillers.
*/
EStateTreeRunStatus FPlayerEngageTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	AOnsetPlayerAIController* Controller = GetPlayerController(Context);
	if (!Controller) return EStateTreeRunStatus::Failed;
	if (!Controller->HasAuthority()) return EStateTreeRunStatus::Failed;
	
	AActor* Target = GetTarget(Context);
	if (!Target)
	{
		return EStateTreeRunStatus::Failed;
	}
	AOnsetBaseCharacter* TargetChar = Cast<AOnsetBaseCharacter>(Target);
	if (!TargetChar || !TargetChar->IsAlive())
	{
		if (UTargetingComponent* TargetingComponent = GetTargetingComponent(Context))                                               
			TargetingComponent->ClearTarget();                                                                                      
		return EStateTreeRunStatus::Failed;   
	}
	
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.EngageStartTime = Controller->GetWorld()->GetTimeSeconds();
	InstanceData.LastAttackTick = 0.0f;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FPlayerEngageTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	AOnsetPlayerAIController* Controller = GetPlayerController(Context);
	if (!Controller) return EStateTreeRunStatus::Failed;
	if (!Controller->HasAuthority()) return EStateTreeRunStatus::Failed;
	
	AOnsetPlayerCharacter* Self = Cast<AOnsetPlayerCharacter>(Controller->GetPawn());
	if (!Self) return EStateTreeRunStatus::Failed;
	
	AActor* Target = GetTarget(Context);
	if (!Target) return EStateTreeRunStatus::Failed;
	AOnsetBaseCharacter* TargetChar = Cast<AOnsetBaseCharacter>(Target);
	if (!TargetChar || !TargetChar->IsAlive())
		return EStateTreeRunStatus::Failed;
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	float Now = Controller->GetWorld()->GetTimeSeconds();

	if (Now - InstanceData.EngageStartTime > InstanceData.MaxEngageDuration) 
		return EStateTreeRunStatus::Failed;
	
	float DistanceSquared = FVector::DistSquared(Self->GetActorLocation(), Target->GetActorLocation());
	
	// --- Phase 1. Approach ---
	if (DistanceSquared > Self->AttackRange * Self->AttackRange)
	{
		Controller->MoveToActor(Target, InstanceData.AcceptanceRadius);
		return EStateTreeRunStatus::Running;
	}
	
	// --- Phase 2. Attack ---
	
	Controller->StopMovement();
	Controller->SetFocus(Target);

	// Throttle: ability selection runs at AttackTickInterval,
	// movement/focus still update every frame
	if (Now - InstanceData.LastAttackTick < InstanceData.AttackTickInterval)
		return EStateTreeRunStatus::Running;
	InstanceData.LastAttackTick = Now;

	UAbilitySystemComponent* ASC = Self->AbilitySystemComponent;
	if (!ASC)
	{
		return EStateTreeRunStatus::Running;
	}

	// --- Gather cooldown-ready attack abilities ---
	struct FScoredAbility
	{
		FGameplayAbilitySpecHandle Handle;
		const UOnsetGA_Generic* Generic = nullptr;         // null = non-generic filler
		const FOnsetAbilityDefinition* Definition = nullptr; // null = no resolvable row
		float Score = 0.0f;           // expected total damage across its hit count
		float CooldownSeconds = 0.0f; // row base; longer CDs win score ties
	};
	TArray<FScoredAbility> Candidates;

	for (FGameplayAbilitySpec& AbilitySpec : ASC->GetActivatableAbilities())
	{
		if (!AbilitySpec.Ability
			|| !AbilitySpec.Ability->CheckCooldown(AbilitySpec.Handle, ASC->AbilityActorInfo.Get())
			|| !AbilitySpec.Ability->GetAssetTags().HasTag(TAG_Ability_Attack))
		{
			continue;
		}

		FScoredAbility Candidate;
		Candidate.Handle = AbilitySpec.Handle;

		if (const UOnsetGA_Generic* Generic = Cast<UOnsetGA_Generic>(AbilitySpec.Ability))
		{
			Candidate.Generic = Generic;
			Candidate.Definition =
				UOnsetAbilityLibrary::GetDefinitionFromDynamicTags(AbilitySpec.DynamicAbilityTags);

			// Refresh gate: an enemy-facing refreshable DoT already active on the primary
			// target would be re-cast to no effect (DoTs refresh, not stack) - drop it.
			// Scoped to THIS caster: another player's live stack of the same DoT must
			// not block our own application (periodic GEs stack per source).
			if (Candidate.Definition
				&& Candidate.Definition->RefreshTag.IsValid()
				&& HasHostilePeriodicDamage(*Candidate.Definition)
				&& UOnsetGA_Generic::HasActivePeriodicInstanceFrom(TargetChar->AbilitySystemComponent,
																  Candidate.Definition->RefreshTag,
																  Self))
			{
				continue;
			}

			if (Candidate.Definition)
			{
				Candidate.CooldownSeconds = Candidate.Definition->CooldownSeconds;
			}
		}
		// Non-data-driven attacks (e.g. GA_BasicAttack): zero-score filler candidates.

		Candidates.Add(Candidate);
	}

	if (Candidates.Num() == 0)
	{
		return EStateTreeRunStatus::Running; // everything on cooldown or refresh-gated
	}

	// --- Single candidate needs no decision (skips the overlap query) ---
	if (Candidates.Num() > 1)
	{
		// Shared overlap around the primary target feeds every multi-target shape.
		TArray<AActor*> OverlapActors;
		{
			TArray<FOverlapResult> Overlaps;
			FCollisionShape Sphere = FCollisionShape::MakeSphere(InstanceData.AoEOverlapRadius);
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(Self);
			Controller->GetWorld()->OverlapMultiByChannel(
				Overlaps,
				Target->GetActorLocation(),
				FQuat::Identity,
				ECC_Pawn,
				Sphere,
				QueryParams);
			for (const FOverlapResult& Hit : Overlaps)
			{
				if (AActor* HitActor = Hit.GetActor())
				{
					if (HitActor->ActorHasTag("Enemy"))
					{
						OverlapActors.Add(HitActor);
					}
				}
			}
		}

		const FVector SelfLoc = Self->GetActorLocation();
		const FVector AimDir = (Target->GetActorLocation() - SelfLoc).GetSafeNormal();

		for (FScoredAbility& Candidate : Candidates)
		{
			if (!Candidate.Generic || !Candidate.Definition)
			{
				continue; // non-generic fillers / unresolvable rows stay at score 0
			}

			int32 Hits = 1;
			switch (Candidate.Definition->AbilityType)
			{
				case EOnsetAbilityType::AoE:
					Hits = CountEnemiesWithin(OverlapActors, Target->GetActorLocation(), Candidate.Definition->Radius);
					break;
				case EOnsetAbilityType::PointBlankAoE:
					Hits = CountEnemiesWithin(OverlapActors, SelfLoc, Candidate.Definition->Radius);
					break;
				case EOnsetAbilityType::Cone:
					Hits = CountConeHits(OverlapActors, SelfLoc, AimDir,
										 Candidate.Definition->Radius, Candidate.Definition->ConeHalfAngle);
					break;
				default:
					break; // SingleTarget / Self always hit exactly 1
			}

			Candidate.Score = Candidate.Generic->GetComparisonDamage(*Candidate.Definition) * Hits;
		}
	}

	// --- Pick best: highest expected damage; ties go to the longest cooldown so the
	// expensive cast goes on cooldown first before falling back to fast fillers.
	int32 BestIndex = INDEX_NONE;
	for (int32 Index = 0; Index < Candidates.Num(); ++Index)
	{
		const FScoredAbility& Candidate = Candidates[Index];
		if (BestIndex == INDEX_NONE)
		{
			BestIndex = Index;
			continue;
		}
		const FScoredAbility& Best = Candidates[BestIndex];
		if (Candidate.Score > Best.Score + KINDA_SMALL_NUMBER
			|| (FMath::IsNearlyEqual(Candidate.Score, Best.Score)
				&& Candidate.CooldownSeconds > Best.CooldownSeconds))
		{
			BestIndex = Index;
		}
	}

	if (BestIndex != INDEX_NONE)
	{
		ASC->TryActivateAbility(Candidates[BestIndex].Handle);
	}
	return EStateTreeRunStatus::Running;
}

void FPlayerEngageTask::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	AOnsetPlayerAIController* Controller = GetPlayerController(Context);
	if (!Controller) return;
	
	AOnsetPlayerCharacter* Self = Cast<AOnsetPlayerCharacter>(Controller->GetPawn());
	if (!Self) return;
	
	Controller->ClearFocus(EAIFocusPriority::Gameplay);
	Controller->StopMovement();
}
