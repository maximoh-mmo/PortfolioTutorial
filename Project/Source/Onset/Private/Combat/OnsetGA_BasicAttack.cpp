// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/OnsetGA_BasicAttack.h"
#include "AbilitySystemComponent.h"
#include "GAS/OnsetGameplayTags.h"
#include "Core/OnsetBaseCharacter.h"
#include "Core/TargetingComponent.h"
#include "UObject/ConstructorHelpers.h"

UOnsetGA_BasicAttack::UOnsetGA_BasicAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	
	static ConstructorHelpers::FObjectFinder<UGameplayEffect> CooldownFinder(                                       
		 TEXT("/Game/Game/Combat/GE_BasicAttackCooldown.GE_BasicAttackCooldown_C"));                                   
	if (CooldownFinder.Succeeded())                                                                                 
	{                                                                                                               
		CooldownGameplayEffectClass = CooldownFinder.Object;                                                        
	}   
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
	if (!TargetActor)
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

	FGameplayAbilityTargetDataHandle TargetData;
	FGameplayAbilityTargetData_ActorArray* ActorArrayData = new FGameplayAbilityTargetData_ActorArray();
	ActorArrayData->TargetActorArray.Add(TargetActor);
	TargetData.Add(ActorArrayData);
	
	// (void) in order to remove IDE warning / suggestion to remove statement as returned Struct is not used.
	auto results = ApplyGameplayEffectToTarget(Handle, ActorInfo, ActivationInfo, TargetData, DamageEffectClass,
	GetAbilityLevel()); 
	
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);                                                 
}                                                                         