// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/OnsetGA_BasicAttack.h"
#include "TimerManager.h"
#include "Combat/OnsetEquipmentLibrary.h"
#include "Data/OnsetEquipmentTypes.h"
#include "GAS/OnsetGameplayTags.h"
#include "Core/OnsetBaseCharacter.h"
#include "Core/TargetingComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

UOnsetGA_BasicAttack::UOnsetGA_BasicAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(TAG_Ability_Attack);
	SetAssetTags(AssetTags);
}

float UOnsetGA_BasicAttack::GetCooldownBaseDuration(const FGameplayAbilitySpecHandle Handle,
													const FGameplayAbilityActorInfo* ActorInfo) const
{
	// The basic attack's base cooldown is the weapon archetype's cooldown
	// (Dagger 0.8s ... Greatsword 1.8s), so the attack rate follows the weapon.
	// Players use their equipped weapon (class default when bare-handed); enemies
	// use the archetype authored in DT_EnemyStats.
	const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	const AOnsetBaseCharacter* Source = Cast<AOnsetBaseCharacter>(Avatar);
	if (!Source)
	{
		return UOnsetEquipmentLibrary::GetArchetypeBaseCooldown(EOnsetWeaponArchetype::Sword);
	}

	return UOnsetEquipmentLibrary::GetArchetypeBaseCooldown(Source->GetBaseWeaponArchetype());
}

void UOnsetGA_BasicAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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
	AActor* TargetActor = Self->TargetingComponent->GetTarget();
	if (!IsValid(TargetActor))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	// Range check at activation
	const float DistSq = FVector::DistSquared(Self->GetActorLocation(), TargetActor->GetActorLocation());
	if (DistSq > FMath::Square(AttackRange))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	// Play montage if available
	if (AttackMontage && Self->GetMesh() && Self->GetMesh()->GetAnimInstance())
	{
		float MontageDuration = Self->PlayAnimMontage(AttackMontage);
		if (MontageDuration > 0.0f)
		{
			// Schedule damage application at DamageTime into montage
			FTimerDelegate TimerDelegate;
			TimerDelegate.BindUObject(this, &UOnsetGA_BasicAttack::ApplyDamageAfterDelay, Handle, ActorInfo, ActivationInfo);
			UWorld* World = Self->GetWorld();
			if (World)
			{
				World->GetTimerManager().SetTimer(MontageTimerHandle, TimerDelegate, DamageTime, false);
			}
			// Don't end ability yet - wait for montage
			return;
		}
	}

	// No montage or failed to play - apply damage immediately
	ApplyDamageAfterDelay(Handle, ActorInfo, ActivationInfo);
}

void UOnsetGA_BasicAttack::ApplyDamageAfterDelay(const FGameplayAbilitySpecHandle Handle,
												 const FGameplayAbilityActorInfo* ActorInfo,
												 const FGameplayAbilityActivationInfo ActivationInfo)
{
	AOnsetBaseCharacter* Self = Cast<AOnsetBaseCharacter>(ActorInfo->AvatarActor);
	if (!Self || !Self->TargetingComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}
	AActor* TargetActor = Self->TargetingComponent->GetTarget();
	if (!IsValid(TargetActor))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	AOnsetBaseCharacter* TargetChar = Cast<AOnsetBaseCharacter>(TargetActor);
	if (!TargetChar || !TargetChar->AbilitySystemComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	// Weapon-scaled physical damage: Raw = WeaponBase x (1 + STR/100). The weapon base
	// comes from the equipped weapon (class-default fallback when bare-handed), scaled
	// by class mastery (e.g. Ranged bow +15%).
	const float Raw = ResolveScaledBase(GetSourceWeaponBase(), EOnsetScalingType::Weapon);
	ApplyDamageToTarget(TargetChar->AbilitySystemComponent, TAG_Damage_Physical, Raw, GetAbilityLevel());

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}