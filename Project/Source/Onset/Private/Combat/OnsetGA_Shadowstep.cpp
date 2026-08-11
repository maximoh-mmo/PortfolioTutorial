// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/OnsetGA_Shadowstep.h"
#include "AbilitySystemComponent.h"
#include "EngineUtils.h"
#include "GAS/OnsetGameplayTags.h"
#include "Core/OnsetBaseCharacter.h"
#include "Enemy/OnsetEnemy.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

UOnsetGA_Shadowstep::UOnsetGA_Shadowstep()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	static ConstructorHelpers::FClassFinder<UGameplayEffect> InvulnFinder(
		TEXT("/Game/Game/Combat/GE_Shadowstep_Invuln.GE_Shadowstep_Invuln_C"));
	if (InvulnFinder.Succeeded())
	{
		InvulnerabilityEffectClass = InvulnFinder.Class;
	}
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(TAG_Ability_Shadowstep);
	SetAssetTags(AssetTags);
}

AActor* UOnsetGA_Shadowstep::FindNearestEnemy(AActor* SourceActor, float MaxDistance) const
{
	if (!SourceActor) return nullptr;

	UWorld* World = SourceActor->GetWorld();
	if (!World) return nullptr;

	FVector SourceLocation = SourceActor->GetActorLocation();
	AActor* BestTarget = nullptr;
	float BestDistSq = FMath::Square(MaxDistance);

	for (TActorIterator<AOnsetEnemy> It(World); It; ++It)
	{
		AOnsetEnemy* Enemy = *It;
		if (!Enemy || !Enemy->IsAlive()) continue;

		float DistSq = FVector::DistSquared(SourceLocation, Enemy->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestTarget = Enemy;
		}
	}

	return BestTarget;
}

void UOnsetGA_Shadowstep::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
										  const FGameplayAbilityActorInfo* ActorInfo,
										  const FGameplayAbilityActivationInfo ActivationInfo,
										  const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	AOnsetBaseCharacter* Self = Cast<AOnsetBaseCharacter>(ActorInfo->AvatarActor);
	if (!Self)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	// Find nearest enemy within distance gate
	AActor* TargetEnemy = FindNearestEnemy(Self, DistanceGate);
	if (!TargetEnemy)
	{
		// No valid target - fizzle (no cooldown, no teleport)
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	// Calculate blink location behind target
	FVector BlinkLocation = TargetEnemy->GetActorLocation() + TargetEnemy->GetActorForwardVector() * -BehindOffset;

	// Teleport the character
	ACharacter* Char = Cast<ACharacter>(Self);
	if (Char && Char->GetCharacterMovement())
	{
		Char->SetActorLocation(BlinkLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}

	// Apply invulnerability effect
	if (InvulnerabilityEffectClass && Self->AbilitySystemComponent)
	{
		FGameplayEffectContextHandle Context = Self->AbilitySystemComponent->MakeEffectContext();
		Context.AddSourceObject(this);
		FGameplayEffectSpecHandle SpecHandle = Self->AbilitySystemComponent->MakeOutgoingSpec(
			InvulnerabilityEffectClass, 1.0f, Context);
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Duration")), InvulnerabilityDuration);
			Self->AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}