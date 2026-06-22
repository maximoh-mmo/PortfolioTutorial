#include "StateTree/Tasks/Player/PlayerEngageTask.h"

#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "StateTreeExecutionContext.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GAS/OnsetGameplayTags.h"
#include "Player/OnsetPlayerCharacter.h"
#include "Core/TargetingComponent.h"

/**
	- Get all granted abilities from ASC (cooldown-aware)                                                                                                                        
	- Count enemies near current target (overlap query)                                                                                                                          
	- Pick: AoE if 3+ enemies, else highest priority single-target ability                                                                                                       
	- TryActivateAbilityByClass on the selected ability                                                                                                                          
	- Return Succeeded on activation, Failed if nothing available  
*/
EStateTreeRunStatus FPlayerEngageTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	AOnsetPlayerAIController* Controller = GetPlayerController(Context);
	if (!Controller) return EStateTreeRunStatus::Failed;
	
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
	TArray<FGameplayAbilitySpec> Abilities = ASC->GetActivatableAbilities();

	// Gather cooldown-ready attack abilities
	TArray<FGameplayAbilitySpecHandle> ReadyAbilities;
	for (auto& AbilitySpec : Abilities)
	{
		if (!AbilitySpec.Ability
			|| !AbilitySpec.Ability->CheckCooldown(AbilitySpec.Handle, ASC->AbilityActorInfo.Get())
			|| !AbilitySpec.Ability->GetAssetTags().HasTag(TAG_Ability_Attack))
			continue;
		ReadyAbilities.Add(AbilitySpec.Handle);
	}

	if (ReadyAbilities.IsEmpty())
		return EStateTreeRunStatus::Running; // wait for cooldown

	// Count nearby enemies only when multiple abilities are ready (needed for AoE decision)
	int32 NearbyEnemies = 0;
	if (ReadyAbilities.Num() > 1)
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
			if (Hit.GetActor() && Hit.GetActor()->ActorHasTag("Enemy"))
				NearbyEnemies++;
		}

		UE_LOG(LogTemp, Log, TEXT("EngageTask: %d abilities ready, %d nearby enemies"),
			ReadyAbilities.Num(), NearbyEnemies);
		// TODO: pick AoE when NearbyEnemies >= 3, else highest-priority single-target
	}

	if (FGameplayAbilitySpec* BestSpec = ASC->FindAbilitySpecFromHandle(ReadyAbilities[0]))
	{
		UE_LOG(LogTemp, Log, TEXT("EngageTask: activating %s"), *BestSpec->Ability->GetName());
		ASC->TryActivateAbility(BestSpec->Handle);
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
