// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/OnsetGA_AoE.h"
#include "AbilitySystemComponent.h"
#include "GAS/OnsetGameplayTags.h"
#include "Core/OnsetBaseCharacter.h"
#include "Core/TargetingComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Player/OnsetPlayerCharacter.h"
#include "Player/OnsetPlayerState.h"

UOnsetGA_AoE::UOnsetGA_AoE()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	static ConstructorHelpers::FObjectFinder<UGameplayEffect> CooldownFinder(
		TEXT("/Game/Game/Combat/GE_AoE_Cooldown.GE_AoE_Cooldown_C"));
	if (CooldownFinder.Succeeded())
	{
		CooldownGameplayEffectClass = CooldownFinder.Object;
	}
	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageFinder(
		TEXT("/Game/Game/Combat/GE_AoE_Damage.GE_AoE_Damage_C"));
	if (DamageFinder.Succeeded())
	{
		DamageEffectClass = DamageFinder.Class;
	}
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(TAG_Ability_AoE);
	SetAssetTags(AssetTags);
}

void UOnsetGA_AoE::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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
	if (!Self || !Self->TargetingComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	// Get target location from targeting component (or use actor location if no target)
	AActor* TargetActor = Self->TargetingComponent->GetTarget();
	FVector TargetLocation = Self->GetActorLocation();
	if (IsValid(TargetActor))
	{
		TargetLocation = TargetActor->GetActorLocation();
	}

	if (!DamageEffectClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	UWorld* World = Self->GetWorld();
	if (!World)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	// Sphere overlap at target location
	TArray<FOverlapResult> OverlapResults;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(AoERadius);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Self);

	bool bHit = World->OverlapMultiByChannel(
		OverlapResults,
		TargetLocation,
		FQuat::Identity,
		OverlapChannel,
		Sphere,
		QueryParams
	);

	if (!bHit)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// Apply damage to each valid target with PvP filtering
	for (const FOverlapResult& Result : OverlapResults)
	{
		AActor* HitActor = Result.GetActor();
		if (!IsValid(HitActor) || HitActor == Self)
		{
			continue;
		}

		// PvP filtering: skip players if PvP disabled
		AOnsetBaseCharacter* HitChar = Cast<AOnsetBaseCharacter>(HitActor);
		if (HitChar && HitChar->IsA(AOnsetPlayerCharacter::StaticClass()))
		{
			// Check if source or target has PvP disabled
			AOnsetPlayerState* SelfPS = Self->GetPlayerState<AOnsetPlayerState>();
			AOnsetPlayerState* TargetPS = HitChar->GetPlayerState<AOnsetPlayerState>();
			if (SelfPS && TargetPS)
			{
				if (!SelfPS->bIsPvPEnabled || !TargetPS->bIsPvPEnabled)
				{
					continue; // PvP disabled for either party
				}
			}
			else if (!SelfPS || !TargetPS)
			{
				// One is not a player (shouldn't happen here), allow damage
			}
		}

		FGameplayAbilityTargetDataHandle TargetData;
		FGameplayAbilityTargetData_ActorArray* ActorArrayData =
			new FGameplayAbilityTargetData_ActorArray();

		ActorArrayData->TargetActorArray.Add(HitActor);
		TargetData.Add(ActorArrayData);

		ApplyGameplayEffectToTarget(Handle, ActorInfo, ActivationInfo, TargetData, DamageEffectClass, GetAbilityLevel());
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}