// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "GameplayTagContainer.h"
#include "OnsetAbilityTypes.generated.h"

class UAnimMontage;
class UOnsetGameplayAbility;
class UNiagaraSystem;
class USoundBase;

/** Which targeting/dispatch shape an ability uses. */
UENUM(BlueprintType)
enum class EOnsetAbilityType : uint8
{
	SingleTarget	UMETA(DisplayName = "Single Target"),
	AoE				UMETA(DisplayName = "AoE"),
	PointBlankAoE	UMETA(DisplayName = "Point Blank AoE"),
	Cone			UMETA(DisplayName = "Cone"),
	Self			UMETA(DisplayName = "Self")
};

/** Optional caster movement executed before an ability resolves. */
UENUM(BlueprintType)
enum class EOnsetAbilityMovementType : uint8
{
	None	UMETA(DisplayName = "None"),
	Leap	UMETA(DisplayName = "Leap")
};

/** Which stat a damage effect's base scales with (see combat-formulas §3). */
UENUM(BlueprintType)
enum class EOnsetScalingType : uint8
{
	/** Raw = WeaponBase x (1 + STR/100). Base = equipped weapon (Magnitude until Phase 2). */
	Weapon	UMETA(DisplayName = "Weapon"),
	/** Raw = SkillBase x (1 + INT/100). Base = the effect's Magnitude. */
	Skill	UMETA(DisplayName = "Skill")
};

/** Damage element of an effect; maps to a Damage.* tag (see combat-formulas §2). */
UENUM(BlueprintType)
enum class EOnsetDamageElement : uint8
{
	Physical	UMETA(DisplayName = "Physical"),
	Fire		UMETA(DisplayName = "Fire"),
	Ice			UMETA(DisplayName = "Ice"),
	Lightning	UMETA(DisplayName = "Lightning"),
	Poison		UMETA(DisplayName = "Poison")
};

/** What a single effect in an ability does. */
UENUM(BlueprintType)
enum class EOnsetAbilityEffectType : uint8
{
	Damage		UMETA(DisplayName = "Damage"),
	Heal		UMETA(DisplayName = "Heal"),
	Snare		UMETA(DisplayName = "Snare"),
	Slow		UMETA(DisplayName = "Slow"),
	Stun		UMETA(DisplayName = "Stun"),
	Freeze		UMETA(DisplayName = "Freeze"),
	Invulnerable	UMETA(DisplayName = "Invulnerable")
};

/**
 * One effect inside an ability row.
 */
USTRUCT(BlueprintType)
struct FOnsetAbilityEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	EOnsetAbilityEffectType Type = EOnsetAbilityEffectType::Damage;

	/** true: affects the caster and allies only; false: affects enemies only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	bool bFriendly = false;

	/** Damage: amount | Snare: move-speed mult (< 1) | Slow: cooldown mult (> 1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	float Magnitude = 0.0f;

	/** 0 = instant (Damage); > 0 = debuff window (Snare/Slow). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	float Duration = 0.0f;

	/** 0 = instant (Damage); > 0 = periodic (DOT/HOT): Magnitude ticks every Period seconds for Duration seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	float Period = 0.0f;

	/** Which stat scales this damage effect. Weapon = STR, Skill = INT. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	EOnsetScalingType ScalingType = EOnsetScalingType::Weapon;

	/** Damage element tag (Damage.Physical/Fire/Ice/Lightning/Poison). Damage effects only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	FGameplayTag DamageTypeTag;
};

/**
 * Composable caster movement that runs before the ability resolves.
 */
USTRUCT(BlueprintType)
struct FOnsetAbilityMovement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	EOnsetAbilityMovementType Type = EOnsetAbilityMovementType::None;

	/** Seconds the leap arc takes to land. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (EditCondition = "Type == EOnsetAbilityMovementType::Leap"))
	float LeapDuration = 0.4f;

	/** Peak height of the arc above the endpoints. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (EditCondition = "Type == EOnsetAbilityMovementType::Leap"))
	float LeapArcHeight = 200.0f;

	/** Max distance the leap can cover; casts beyond this fizzle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (EditCondition = "Type == EOnsetAbilityMovementType::Leap"))
	float LeapMaxRange = 800.0f;
};

/** One row in DT_Abilities. RowName is the stable ability ID (e.g. "AoE", "Cone"). */
USTRUCT(BlueprintType)
struct FOnsetAbilityDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	TSoftObjectPtr<UTexture2D> AbilityIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	EOnsetAbilityType AbilityType = EOnsetAbilityType::SingleTarget;

	/** C++ class implementing the behavior. Behavior lives in C++; the table holds parameters only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability", meta = (AllowedClasses = "/Script/Onset.OnsetGameplayAbility"))
	TSoftClassPtr<UOnsetGameplayAbility> AbilityClass;

	/** GAS input ID; -1 = unbound/passive. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	int32 InputID = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float CooldownSeconds = 1.0f;

	/** Cooldown tag granted while this ability is on cooldown (Cooldown.<RowName>). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	FGameplayTag CooldownTag;

	/**
	 * Granted on this ability's periodic applications (DoT/HoT specs) so consumers can
	 * detect an active instance on a target. AI ability selection uses it as a refresh
	 * gate: a refresh-style DoT scores 0 while its RefreshTag is present on the target.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	FGameplayTag RefreshTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability", meta = (EditCondition = "AbilityType == EOnsetAbilityType::SingleTarget"))
	float AttackRange = 300.0f;

	/** Max cast distance for targeted AoE (range-gated at activation). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability", meta = (EditCondition = "AbilityType == EOnsetAbilityType::AoE"))
	float CastRange = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability", meta = (EditCondition = "AbilityType == EOnsetAbilityType::AoE || AbilityType == EOnsetAbilityType::PointBlankAoE || AbilityType == EOnsetAbilityType::Cone"))
	float Radius = 300.0f;

	/** Optional caster movement executed before the ability resolves. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	FOnsetAbilityMovement Movement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability", meta = (EditCondition = "AbilityType == EOnsetAbilityType::Cone"))
	float ConeHalfAngle = 90.0f;

	/** Optional attack montage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	TSoftObjectPtr<UAnimMontage> Montage;

	/** Optional montage hit time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float DamageTime = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	TArray<FOnsetAbilityEffect> Effects;

	/** Multiplier on threat generated by this ability's damage (1.0 = normal). High-threat attacks use >1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Threat")
	float ThreatMultiplier = 1.0f;

	/** Optional Niagara system spawned on the caster when casting starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual & Audio Cues")
	TSoftObjectPtr<UNiagaraSystem> CastNiagara;

	/** Optional Niagara system spawned at the target / hit impact location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual & Audio Cues")
	TSoftObjectPtr<UNiagaraSystem> ImpactNiagara;

	/** Optional sound played on cast start. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual & Audio Cues")
	TSoftObjectPtr<USoundBase> CastSound;

	/** Optional sound played on hit / impact. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual & Audio Cues")
	TSoftObjectPtr<USoundBase> ImpactSound;
};
