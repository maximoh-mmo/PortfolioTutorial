// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/OnsetGA_CooldownSlow.h"

#include "AbilitySystemComponent.h"
#include "Core/OnsetBaseCharacter.h"
#include "Core/TargetingComponent.h"
#include "GAS/OnsetCooldownSlowEffect.h"
#include "GAS/OnsetGameplayTags.h"

UOnsetGA_CooldownSlow::UOnsetGA_CooldownSlow()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	SlowEffectClass = UOnsetCooldownSlowEffect::StaticClass();

	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(TAG_Ability_Debuff);
	SetAssetTags(AssetTags);
}

void UOnsetGA_CooldownSlow::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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
	if (!Self || !Self->TargetingComponent || !SlowEffectClass)
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

	// Range check at activation (same pattern as the basic attack).
	const float DistSq = FVector::DistSquared(Self->GetActorLocation(), TargetActor->GetActorLocation());
	if (DistSq > FMath::Square(AbilityRange))
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

	FGameplayEffectContextHandle Context = Self->AbilitySystemComponent->MakeEffectContext();
	Context.AddInstigator(Self, nullptr);
	FGameplayEffectSpecHandle SpecHandle = Self->AbilitySystemComponent->MakeOutgoingSpec(
		SlowEffectClass, GetAbilityLevel(), Context);
	if (!SpecHandle.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(FName("CooldownRateMod"), CooldownRateMod);
	SpecHandle.Data->SetSetByCallerMagnitude(FName("Duration"), SlowDuration);
	TargetChar->AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetChar->AbilitySystemComponent);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
