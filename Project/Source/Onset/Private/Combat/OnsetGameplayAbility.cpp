// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/OnsetGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "Core/OnsetBaseCharacter.h"
#include "GAS/OnsetCombatAttributeSet.h"
#include "GAS/OnsetGameplayTags.h"
#include "GAS/OnsetGenericDamageEffect.h"
#include "GameplayTagContainer.h"

FGameplayTag UOnsetGameplayAbility::GetPrimaryCooldownTag() const
{
	const FGameplayTagContainer* CooldownTags = GetCooldownTags();
	if (CooldownTags && CooldownTags->Num() > 0)
	{
		return CooldownTags->First();
	}
	return FGameplayTag();
}

void UOnsetGameplayAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
										  const FGameplayAbilityActorInfo* ActorInfo,
										  const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	if (!CooldownGE || !ActorInfo)
	{
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		return;
	}

	const float Level = GetAbilityLevel(Handle, ActorInfo);
	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CooldownGE->GetClass(), Level);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	// Cooldowns are fully GE-driven in UE 5.8 (no CooldownTime on the ability),
	// so the base duration is the cooldown GE's static duration magnitude.
	float BaseDuration = 0.0f;
	if (CooldownGE->DurationMagnitude.GetStaticMagnitudeIfPossible(Level, BaseDuration))
	{
		// Scale by the source character's CooldownMultiplier: a Slow debuff raises
		// it above 1, extending the cooldown so the target attacks less often.
		float Multiplier = 1.0f;
		if (const AActor* Avatar = ActorInfo->AvatarActor.Get())
		{
			if (const AOnsetBaseCharacter* Source = Cast<AOnsetBaseCharacter>(Avatar))
			{
				if (Source->CombatAttributes)
				{
					Multiplier = Source->CombatAttributes->GetCooldownMultiplier();
				}
			}
		}

		// Guard against a zero/negative multiplier from a misconfigured debuff:
		// never allow the cooldown to collapse or go negative.
		Multiplier = FMath::Max(0.1f, Multiplier);
		const float FinalDuration = FMath::Max(0.1f, BaseDuration * Multiplier);
		SpecHandle.Data->SetDuration(FinalDuration, true);
	}
	else
	{
		// Non-static duration (e.g. SetByCaller) can't be scaled here; the cooldown
		// still applies at its configured duration, but the multiplier is skipped.
		UE_LOG(LogTemp, Warning, TEXT("UOnsetGameplayAbility::ApplyCooldown: cooldown GE '%s' has a non-static duration; CooldownMultiplier scaling skipped."), *GetNameSafe(CooldownGE));
	}

	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}

void UOnsetGameplayAbility::ApplyDamageToTarget(UAbilitySystemComponent* TargetASC,
												float Physical,
												float Magical,
												float Level) const
{
	if (!TargetASC)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
	{
		return;
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	if (AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		Context.AddInstigator(Avatar, Avatar->GetInstigatorController());
	}

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(
		UOnsetGenericDamageEffect::StaticClass(), Level, Context);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(TAG_Damage_Physical, Physical);
	SpecHandle.Data->SetSetByCallerMagnitude(TAG_Damage_Magical, Magical);

	TargetASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}
