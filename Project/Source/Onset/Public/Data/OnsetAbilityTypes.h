// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "GameplayTagContainer.h"
#include "OnsetAbilityTypes.generated.h"

class UOnsetGameplayAbility;

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

/** What a single effect in an ability does. */
UENUM(BlueprintType)
enum class EOnsetAbilityEffectType : uint8
{
	Damage		UMETA(DisplayName = "Damage"),
	Heal		UMETA(DisplayName = "Heal"),
	Snare		UMETA(DisplayName = "Snare"),
	Slow		UMETA(DisplayName = "Slow"),
	Stun		UMETA(DisplayName = "Stun"),
	Invulnerable	UMETA(DisplayName = "Invulnerable")
};

/**
 * One effect inside an ability row.
 *
 * - Damage: Magnitude is applied to the target's Health via GE_GenericDamage
 *   (SetByCaller Damage.Physical / Damage.Magical). Duration is ignored.
 * - Heal: Magnitude is applied to the target's Health (positive, clamped to
 *   MaxHealth) via GE_GenericHeal. Duration is ignored.
 * - Damage/Heal with Period > 0: becomes a Damage-over-Time / Heal-over-Time effect
 *   (GE_GenericDamageOT / GE_GenericHealOT). Magnitude ticks every Period seconds
 *   for the whole Duration window.
 * - Snare: Magnitude is the MovementSpeed multiply-compound (MoveSpeedMod, < 1
 *   slows). Has Duration.
 * - Slow: Magnitude is the CooldownMultiplier multiply-compound (CooldownRateMod,
 *   > 1 slows attack rate). Has Duration.
 * - Stun: Duration is the stun window granted as State.Stunned (blocks ability
 *   activation for the caster). Magnitude is ignored.
 * - Invulnerable: Duration is the window granted as State.Invulnerable (negates
 *   all damage). Magnitude is ignored.
 *
 * Friendliness gates who can receive an effect:
 * - bFriendly = true: applies to the caster and allies only (Heal, self/ally
 *   buffs like Invulnerable).
 * - bFriendly = false: applies to enemies only (Damage, Snare, Slow, Stun).
 * Alignment is derived from the character class (player vs non-player) plus the
 * PvP toggle; see UOnsetGA_Generic::ShouldAffectActor / IsFriendlyActor.
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

	/** 0 = instant (Damage/Heal); > 0 = periodic (DOT/HOT): Magnitude ticks every Period seconds for Duration seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	float Period = 0.0f;

	/** Damage.Physical | Damage.Magical (Damage effects only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	FGameplayTag DamageTypeTag;
};

/**
 * Composable caster movement that runs before the ability resolves.
 *
 * - Leap: a code-driven parabolic arc toward the current target (landing spot is
 *   the target's location captured at activation). LeapMaxRange gates the cast
 *   (fizzle if the target is beyond it).
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
};
