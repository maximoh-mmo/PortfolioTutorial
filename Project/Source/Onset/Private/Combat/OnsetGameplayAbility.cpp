// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/OnsetGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "Combat/OnsetEquipmentLibrary.h"
#include "Core/OnsetBaseCharacter.h"
#include "Data/OnsetEquipmentTypes.h"
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

	// Cooldowns are fully GE-driven in UE 5.8 (no CooldownTime on the ability).
	// Base duration comes from the cooldown GE's static magnitude (or an override
	// such as the basic attack's weapon-archetype cooldown).
	const float BaseDuration = GetCooldownBaseDuration(Handle, ActorInfo);
	if (BaseDuration > 0.0f)
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

		// Haste/CDR shortens the base: EffectiveCooldown = Base x (1 - TotalCDR%) x Multiplier.
		const float FinalDuration = FMath::Max(0.1f,
			BaseDuration * (1.0f - GetTotalCooldownReduction()) * FMath::Max(0.1f, Multiplier));
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
												FGameplayTag DamageTypeTag,
												float Amount,
												float Level) const
{
	if (!TargetASC || !DamageTypeTag.IsValid())
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

	SpecHandle.Data->SetSetByCallerMagnitude(DamageTypeTag, Amount);

	TargetASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

const UOnsetCombatAttributeSet* UOnsetGameplayAbility::GetSourceCombatAttributes() const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	const AOnsetBaseCharacter* Source = Cast<AOnsetBaseCharacter>(Avatar);
	return Source ? Source->CombatAttributes : nullptr;
}

float UOnsetGameplayAbility::GetSourceWeaponBase() const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	const AOnsetBaseCharacter* Source = Cast<AOnsetBaseCharacter>(Avatar);
	if (!Source)
	{
		return 25.0f;
	}

	float Base = Source->GetEquippedWeaponDamage();

	// Ranged mastery: bow damage +15% (combat-formulas §11).
	const FOnsetEquipmentDefinition* Weapon = Source->GetEquippedItem(EOnsetEquipmentSlot::Weapon);
	if (Source->GetCharacterClass() == EOnsetCharacterClass::Ranged &&
		Weapon && Weapon->Archetype == EOnsetWeaponArchetype::Bow)
	{
		Base *= 1.0f + UOnsetEquipmentLibrary::GetBowMasteryDamageBonus();
	}

	return Base;
}

float UOnsetGameplayAbility::GetBuffPotency() const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	const AOnsetBaseCharacter* Source = Cast<AOnsetBaseCharacter>(Avatar);
	if (!Source || Source->GetCharacterClass() != EOnsetCharacterClass::Support)
	{
		return 1.0f;
	}

	// SupportMasteryPotencyBonus = 0.20 (combat-formulas §14).
	constexpr float SupportMasteryPotencyBonus = 0.20f;
	return 1.0f + SupportMasteryPotencyBonus;
}

float UOnsetGameplayAbility::GetTotalCooldownReduction() const
{
	const UOnsetCombatAttributeSet* Combat = GetSourceCombatAttributes();
	if (!Combat)
	{
		return 0.0f;
	}

	float Total = 0.0f;

	// Haste% = AGI/(AGI + K_haste), K_haste = 200 (combat-formulas §14).
	constexpr float K_Haste = 200.0f;
	const float Agility = FMath::Max(0.0f, Combat->GetAgility());
	Total += Agility / (Agility + K_Haste);

	// Dual-wield CDR: a melee weapon with an empty off-hand is treated as dual-wielding.
	// DualWieldCDRBonus = 20% base + 15% for the MeleeDPS (DPS class) mastery.
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (const AOnsetBaseCharacter* Source = Cast<AOnsetBaseCharacter>(Avatar))
	{
		const FOnsetEquipmentDefinition* Weapon = Source->GetEquippedItem(EOnsetEquipmentSlot::Weapon);
		const bool bNoShield = !Source->GetEquippedItem(EOnsetEquipmentSlot::Shield);
		if (Weapon && UOnsetEquipmentLibrary::IsMeleeArchetype(Weapon->Archetype) && bNoShield)
		{
			Total += UOnsetEquipmentLibrary::GetDualWieldBaseCDR();
			if (Source->GetCharacterClass() == EOnsetCharacterClass::DPS)
			{
				Total += UOnsetEquipmentLibrary::GetMeleeDPSBonusCDR();
			}
		}
	}

	return FMath::Clamp(Total, 0.0f, 0.8f);
}

float UOnsetGameplayAbility::GetCooldownBaseDuration(const FGameplayAbilitySpecHandle Handle,
													 const FGameplayAbilityActorInfo* ActorInfo) const
{
	const UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	if (!CooldownGE)
	{
		return 0.0f;
	}

	const float Level = GetAbilityLevel(Handle, ActorInfo);
	float BaseDuration = 0.0f;
	if (CooldownGE->DurationMagnitude.GetStaticMagnitudeIfPossible(Level, BaseDuration))
	{
		return BaseDuration;
	}
	return 0.0f;
}

float UOnsetGameplayAbility::ResolveScaledBase(float Base, EOnsetScalingType ScalingType) const
{
	const UOnsetCombatAttributeSet* CombatAttributes = GetSourceCombatAttributes();
	if (!CombatAttributes)
	{
		return Base;
	}

	const float Stat = (ScalingType == EOnsetScalingType::Skill)
		? CombatAttributes->GetIntellect()
		: CombatAttributes->GetStrength();

	// STR_Divisor = INT_Divisor = 100 (combat-formulas §14); +100 stat doubles the base.
	return Base * (1.0f + FMath::Max(0.0f, Stat) / 100.0f);
}
