// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/GameplayAbilities/OnsetGA_BasicAttack.h"
#include "AbilitySystemComponent.h"
#include "Player/OnsetBaseCharacter.h"
#include "Player/TargetingComponent.h"
#include "UObject/ConstructorHelpers.h"

UOnsetGA_BasicAttack::UOnsetGA_BasicAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	
	static ConstructorHelpers::FClassFinder<UGameplayEffect> CooldownFinder(
		TEXT("/Game/Game/Combat/GE_BasicAttackCooldown.GE_BasicAttackCooldown_C"));
	if (CooldownFinder.Succeeded())
	{
		CooldownGameplayEffectClass = CooldownFinder.Class;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("CooldownFinder failed"));
	}
	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageFinder(
		TEXT("/Game/Game/Combat/GE_BasicAttackDamage.GE_BasicAttackDamage_C"));
	if (DamageFinder.Succeeded())
	{
		DamageEffectClass = DamageFinder.Class;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("DamageFinder failed"));
	}
	
}

void UOnsetGA_BasicAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                           const FGameplayAbilityActorInfo* ActorInfo, 
                                           const FGameplayAbilityActivationInfo ActivationInfo,
                                           const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Warning, TEXT("BasicAttack: CommitAbility failed"));
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}
	
	AOnsetBaseCharacter* Self = Cast<AOnsetBaseCharacter>(ActorInfo->AvatarActor);
	if (!Self || !Self->TargetingComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("BasicAttack: Invalid self or targeting component"));
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}
	AActor* TargetActor = Self->TargetingComponent->GetTarget();
	if (!TargetActor)	{
		UE_LOG(LogTemp, Warning, TEXT("BasicAttack: No target found"));
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}
	
	// Range check at activation
	const float DistSq = FVector::DistSquared(Self->GetActorLocation(), TargetActor->GetActorLocation());
	if (DistSq > FMath::Square(AttackRange))
	{
		UE_LOG(LogTemp, Warning, TEXT("BasicAttack: Target out of range"));
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}
	
	if (!DamageEffectClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("BasicAttack: Damage effect class not found"));
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
	
	UE_LOG(LogTemp, Log, TEXT("BasicAttack: Damage applied to %s"), *TargetActor->GetName());                                                                                     
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);                                                 
}                                                                         