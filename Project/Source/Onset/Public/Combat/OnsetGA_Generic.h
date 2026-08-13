// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Combat/OnsetGameplayAbility.h"
#include "OnsetGA_Generic.generated.h"

class AOnsetBaseCharacter;
class UAbilitySystemComponent;
struct FOnsetAbilityDefinition;
struct FOnsetAbilityEffect;

/**
 * Data-driven ability runtime.
 *
 * Resolves its DT_Abilities row from the spec's DynamicAbilityTags (AbilityID.<RowName>)
 * and dispatches on the row's AbilityType: SingleTarget (targeting-component target),
 * AoE (sphere overlap), PointBlankAoE (self-centered sphere), or Cone (directional
 * overlap). Each effect in the row is applied through the shared templates: Damage via
 * GE_GenericDamage (SetByCaller), Snare via GE_GenericSnare, Slow via GE_GenericSlow.
 * Cooldowns are applied from the row's CooldownSeconds via GE_GenericCooldown with the
 * Cooldown.<RowName> tag added dynamically.
 *
 * Validation runs BEFORE CommitAbility so every failed cast (no target, out of range,
 * blocked LoS) fizzles WITHOUT consuming cost or cooldown. The optional Movement.Leap
 * runs a code-driven parabolic arc to the current target before the type resolves, and
 * is structured so it can later be replaced by animation-driven root motion without
 * touching the dispatch.
 */
UCLASS()
class ONSET_API UOnsetGA_Generic : public UOnsetGameplayAbility
{
	GENERATED_BODY()

public:
	UOnsetGA_Generic();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
							   const FGameplayAbilityActorInfo* ActorInfo,
							   const FGameplayAbilityActivationInfo ActivationInfo) const override;

private:
	/**
	 * Pre-commit validation gate for the resolved row. Returns false (and the cast
	 * fizzles with no cooldown) when the target is missing, out of range, or has no
	 * line of sight. Leap movement gates on the leap range instead of the type range.
	 */
	bool ValidateActivation(AOnsetBaseCharacter* Self,
							const FOnsetAbilityDefinition& Definition,
							AActor* TargetActor) const;

	/** True when a straight visibility trace from Self to TargetActor is unobstructed. */
	bool HasLineOfSight(AOnsetBaseCharacter* Self, const AActor* TargetActor) const;

	/** True unless PvP filtering should exclude HitChar from Self's attack. */
	bool ShouldAffectActor(AOnsetBaseCharacter* Self, AOnsetBaseCharacter* HitChar) const;

	/** Applies every effect in Definition->Effects to TargetASC. */
	void ApplyEffects(const FOnsetAbilityDefinition& Definition,
					  UAbilitySystemComponent* TargetASC,
					  float Level) const;

	/** Applies a single effect to TargetASC (Damage/Snare/Slow). */
	void ApplyEffect(const FOnsetAbilityEffect& Effect,
					 UAbilitySystemComponent* TargetASC,
					 float Level) const;

	/** Returns the row definition resolved from the spec's DynamicAbilityTags. */
	const FOnsetAbilityDefinition* ResolveDefinition(const FGameplayAbilitySpecHandle Handle,
													 const FGameplayAbilityActorInfo* ActorInfo) const;

	/** Applies a GE to TargetASC with the given SetByCaller magnitudes (name -> value). */
	void ApplyEffectSpecToTarget(TSubclassOf<UGameplayEffect> EffectClass,
								 UAbilitySystemComponent* TargetASC,
								 const TMap<FName, float>& SetByCallerMagnitudes,
								 float Level) const;

	/** Type-specific resolve of the ability (after commit / after any leap lands). */
	void ResolveAbility(const FOnsetAbilityDefinition& Definition,
						AOnsetBaseCharacter* Self,
						AActor* TargetActor,
						const FGameplayAbilitySpecHandle Handle,
						const FGameplayAbilityActorInfo* ActorInfo,
						const FGameplayAbilityActivationInfo ActivationInfo);

	/** Applies effects to every valid character overlapping Origin with the row's Radius. */
	void ApplyAoEAtLocation(const FOnsetAbilityDefinition& Definition,
							AOnsetBaseCharacter* Self,
							const FVector& Origin);

	// --- Leap (code-driven arc; swap point for animation/root-motion later) ---

	/** Starts the parabolic leap toward Destination; ResolveAbility runs on landing. */
	void PerformLeap(const FOnsetAbilityDefinition& Definition,
					 AOnsetBaseCharacter* Self,
					 const FVector& Destination,
					 const FGameplayAbilitySpecHandle Handle,
					 const FGameplayAbilityActorInfo* ActorInfo,
					 const FGameplayAbilityActivationInfo ActivationInfo);

	/** Repeating-timer tick advancing the arc. */
	void TickLeap();

	/** Ran once the arc completes: snap to landing, restore movement, resolve. */
	void OnLeapFinished(const FGameplayAbilitySpecHandle Handle,
						const FGameplayAbilityActorInfo* ActorInfo,
						const FGameplayAbilityActivationInfo ActivationInfo);

	/** Home + target location captured at leap start. */
	UPROPERTY()
	FVector LeapStartLocation = FVector::ZeroVector;

	UPROPERTY()
	FVector LeapDestinationLocation = FVector::ZeroVector;

	float LeapElapsedSeconds = 0.0f;
	float LeapDurationSeconds = 0.0f;
	float LeapArcHeight = 0.0f;

	/** EMovementMode to restore on landing (saved before DisableMovement). */
	UPROPERTY()
	uint8 SavedMovementMode = 0;

	/** Repeating timer advancing the leap arc. */
	UPROPERTY()
	FTimerHandle LeapTimerHandle;

	/** Completion timer firing OnLeapFinished after LeapDurationSeconds. */
	UPROPERTY()
	FTimerHandle LeapCompletionTimerHandle;

	/** Timer handler for the delayed single-target montage hit. */
	FTimerHandle MontageTimerHandle;

	/** Target ASC captured at activation for the delayed damage tick. */
	TWeakObjectPtr<UAbilitySystemComponent> CachedTargetASC;

	void ApplyCachedDamageAfterDelay(const FGameplayAbilitySpecHandle Handle,
									 const FGameplayAbilityActorInfo* ActorInfo,
									 const FGameplayAbilityActivationInfo ActivationInfo);
};
