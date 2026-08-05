// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/ExecCalc_Stagger.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "TimerManager.h"
#include "Core/OnsetBaseCharacter.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GAS/OnsetGameplayTags.h"

UExecCalc_Stagger::UExecCalc_Stagger()
{
	// Define attributes we might capture (none needed for pure execution)
	RelevantAttributesToCapture.Empty();
}

void UExecCalc_Stagger::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	// Get source and target actors
	UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();

	AActor* SourceActor = SourceASC ? SourceASC->GetAvatarActor() : nullptr;
	AActor* TargetActor = TargetASC ? TargetASC->GetAvatarActor() : nullptr;

	if (!TargetActor)
	{
		return;
	}

	AOnsetBaseCharacter* TargetChar = Cast<AOnsetBaseCharacter>(TargetActor);
	if (!TargetChar || !TargetChar->IsAlive())
	{
		return;
	}

	// Check for invulnerability tag (Shadowstep grants this)
	if (TargetASC && TargetASC->HasMatchingGameplayTag(TAG_State_Invulnerable))
	{
		return; // Invulnerable - no stagger
	}

	// Apply knockback
	ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor);
	if (TargetCharacter && TargetCharacter->GetCharacterMovement() && SourceActor)
	{
		FVector KnockbackDir = (TargetActor->GetActorLocation() - SourceActor->GetActorLocation()).GetSafeNormal();
		if (KnockbackDir.IsNearlyZero())
		{
			KnockbackDir = TargetActor->GetActorForwardVector() * -1.0f;
		}
		
		const float KnockbackMagnitude = 500.0f;
		TargetCharacter->LaunchCharacter(KnockbackDir * KnockbackMagnitude, true, true);
	}

	// Apply global time dilation (hitstop) - 0.1x for 0.1s
	// This is a global effect, so we use a timer to restore
	UWorld* World = TargetActor->GetWorld();
	if (World)
	{
		UGameplayStatics::SetGlobalTimeDilation(World, 0.1f);
		
		FTimerHandle TimerHandle;
		World->GetTimerManager().SetTimer(TimerHandle, [World]()
		{
			UGameplayStatics::SetGlobalTimeDilation(World, 1.0f);
		}, 0.1f, false);
	}

	// Grant stagger tag (handled by the GE's granted tags, but we ensure it here)
	if (TargetASC)
	{
		FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
		Context.AddSourceObject(this);
		// The GE_Stagger already grants TAG_State_Staggered via its granted tags
		// This execution just adds the physical effects
	}
}