// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/OnsetGA_BasicAttack.h"
#include "TimerManager.h"
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

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageFinder(
		TEXT("/Game/Game/Combat/GE_BasicAttackDamage.GE_BasicAttackDamage_C"));
	if (DamageFinder.Succeeded())
	{
		DamageEffectClass = DamageFinder.Class;
	}
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(TAG_Ability_Attack);
	SetAssetTags(AssetTags);
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

	if (!DamageEffectClass)
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

	FGameplayAbilityTargetDataHandle TargetData;
	FGameplayAbilityTargetData_ActorArray* ActorArrayData = new FGameplayAbilityTargetData_ActorArray();
	ActorArrayData->TargetActorArray.Add(TargetActor);
	TargetData.Add(ActorArrayData);

	(void)ApplyGameplayEffectToTarget(Handle, ActorInfo, ActivationInfo, TargetData, DamageEffectClass, GetAbilityLevel());

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}