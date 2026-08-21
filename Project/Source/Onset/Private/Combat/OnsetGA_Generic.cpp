// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/OnsetGA_Generic.h"

#include "AbilitySystemComponent.h"
#include "Core/OnsetBaseCharacter.h"
#include "Core/OnsetCCDiminishingComponent.h"
#include "Core/TargetingComponent.h"
#include "Data/OnsetAbilityTypes.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GAS/OnsetCombatAttributeSet.h"
#include "GAS/OnsetCooldownSlowEffect.h"
#include "GAS/OnsetGenericCooldownEffect.h"
#include "GAS/OnsetGenericDamageEffect.h"
#include "GAS/OnsetGenericDamageOverTimeEffect.h"
#include "GAS/OnsetGenericFreezeEffect.h"
#include "GAS/OnsetGenericHealEffect.h"
#include "GAS/OnsetGenericHealOverTimeEffect.h"
#include "GAS/OnsetGenericInvulnerableEffect.h"
#include "GAS/OnsetGenericSnareEffect.h"
#include "GAS/OnsetGenericStunEffect.h"
#include "GAS/OnsetGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Player/OnsetPlayerCharacter.h"
#include "Player/OnsetPlayerState.h"
#include "TimerManager.h"
#include "Combat/OnsetAbilityLibrary.h"

UOnsetGA_Generic::UOnsetGA_Generic()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

const FOnsetAbilityDefinition* UOnsetGA_Generic::ResolveDefinition(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!ActorInfo)
	{
		return nullptr;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		return nullptr;
	}

	const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
	if (!Spec)
	{
		return nullptr;
	}

	return UOnsetAbilityLibrary::GetDefinitionFromDynamicTags(Spec->DynamicAbilityTags);
}

void UOnsetGA_Generic::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
									   const FGameplayAbilityActorInfo* ActorInfo,
									   const FGameplayAbilityActivationInfo ActivationInfo,
									   const FGameplayEventData* TriggerEventData)
{
	AOnsetBaseCharacter* Self = Cast<AOnsetBaseCharacter>(ActorInfo->AvatarActor);
	if (!Self)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	const FOnsetAbilityDefinition* Definition = ResolveDefinition(Handle, ActorInfo);
	if (!Definition)
	{
		UE_LOG(LogTemp, Error, TEXT("UOnsetGA_Generic: unable to resolve definition for ability %s"), *GetNameSafe(GetClass()));
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	CachedThreatMultiplier = FMath::Max(0.0f, Definition->ThreatMultiplier);

	AActor* TargetActor = Self->TargetingComponent ? Self->TargetingComponent->GetTarget() : nullptr;

	// Pre-commit validation: a failed cast (no target, out of range, blocked LoS)
	// fizzles WITHOUT applying cost or cooldown.
	if (!ValidateActivation(Self, *Definition, TargetActor))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	// Optional composable leap movement toward the current target before the resolve.
	if (Definition->Movement.Type == EOnsetAbilityMovementType::Leap)
	{
		if (!IsValid(TargetActor))
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
			return;
		}

		PerformLeap(*Definition, Self, TargetActor->GetActorLocation(), Handle, ActorInfo, ActivationInfo);
		return;
	}

	ResolveAbility(*Definition, Self, TargetActor, Handle, ActorInfo, ActivationInfo);
}

bool UOnsetGA_Generic::ValidateActivation(AOnsetBaseCharacter* Self,
										  const FOnsetAbilityDefinition& Definition,
										  AActor* TargetActor) const
{
	if (!Self)
	{
		return false;
	}

	// Stun/Freeze gate: a hard-CC'd caster cannot start abilities.
	if (Self->AbilitySystemComponent &&
		(Self->AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Stunned) ||
		 Self->AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Frozen)))
	{
		return false;
	}

	// Leap movement gates purely on the leap range against the target.
	if (Definition.Movement.Type == EOnsetAbilityMovementType::Leap)
	{
		if (!IsValid(TargetActor))
		{
			return false;
		}

		const float DistSq = FVector::DistSquared(Self->GetActorLocation(), TargetActor->GetActorLocation());
		if (DistSq > FMath::Square(Definition.Movement.LeapMaxRange))
		{
			return false;
		}

		return HasLineOfSight(Self, TargetActor);
	}

	switch (Definition.AbilityType)
	{
		case EOnsetAbilityType::SingleTarget:
		{
			AOnsetBaseCharacter* TargetChar = Cast<AOnsetBaseCharacter>(TargetActor);
			if (!TargetChar || !TargetChar->AbilitySystemComponent)
			{
				return false;
			}

			const float DistSq = FVector::DistSquared(Self->GetActorLocation(), TargetActor->GetActorLocation());
			if (DistSq > FMath::Square(Definition.AttackRange))
			{
				return false;
			}

			if (!HasLineOfSight(Self, TargetActor))
			{
				return false;
			}

			// Every effect must have a resolvable recipient: hostile effects need a
			// hostile target, friendly effects a friendly recipient (direct ally or TT).
			if (Definition.Effects.Num() > 0)
			{
				bool bHasHostile = false;
				bool bHasFriendly = false;
				for (const FOnsetAbilityEffect& Effect : Definition.Effects)
				{
					if (Effect.bFriendly) bHasFriendly = true;
					else bHasHostile = true;
				}

				if (bHasHostile && !ShouldAffectActor(Self, TargetChar))
				{
					return false;
				}
				if (bHasFriendly && !ResolveHealRecipient(Self, TargetChar))
				{
					return false;
				}
			}

			return true;
		}

		case EOnsetAbilityType::AoE:
		{
			AOnsetBaseCharacter* TargetChar = Cast<AOnsetBaseCharacter>(TargetActor);
			if (!TargetChar)
			{
				return false;
			}

			const float DistSq = FVector::DistSquared(Self->GetActorLocation(), TargetActor->GetActorLocation());
			if (DistSq > FMath::Square(Definition.CastRange))
			{
				return false;
			}

			return HasLineOfSight(Self, TargetActor);
		}

		case EOnsetAbilityType::PointBlankAoE:
		case EOnsetAbilityType::Cone:
		case EOnsetAbilityType::Self:
			// Self-centered or caster-targeted: no target, range, or LoS gate.
			return true;

		default:
			return false;
	}
}

bool UOnsetGA_Generic::HasLineOfSight(AOnsetBaseCharacter* Self, const AActor* TargetActor) const
{
	if (!Self || !IsValid(TargetActor))
	{
		return false;
	}

	UWorld* World = Self->GetWorld();
	if (!World)
	{
		return false;
	}

	// Trace at eye height so the floor and the target's own capsule don't block it.
	const FVector Start = Self->GetActorLocation() + FVector(0.0f, 0.0f, 90.0f);
	const FVector End = TargetActor->GetActorLocation() + FVector(0.0f, 0.0f, 90.0f);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OnsetAbilityLineOfSight), false);
	QueryParams.AddIgnoredActor(Self);
	QueryParams.AddIgnoredActor(TargetActor);

	FHitResult Hit;
	return !World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams);
}

bool UOnsetGA_Generic::ShouldAffectActor(AOnsetBaseCharacter* Self, AOnsetBaseCharacter* HitChar) const
{
	if (!Self || !HitChar || HitChar == Self)
	{
		return false;
	}

	const bool bSelfIsPlayer = Self->IsA(AOnsetPlayerCharacter::StaticClass());
	const bool bHitIsPlayer = HitChar->IsA(AOnsetPlayerCharacter::StaticClass());

	// Opposite sides (player vs non-player) are always enemies.
	if (bSelfIsPlayer != bHitIsPlayer)
	{
		return true;
	}

	// Same side, non-players: allies (enemies don't friendly-fire each other).
	if (!bSelfIsPlayer)
	{
		return false;
	}

	// Both players: enemies only when both have PvP enabled.
	AOnsetPlayerState* SelfPS = Self->GetPlayerState<AOnsetPlayerState>();
	AOnsetPlayerState* TargetPS = HitChar->GetPlayerState<AOnsetPlayerState>();
	return SelfPS && TargetPS && SelfPS->bIsPvPEnabled && TargetPS->bIsPvPEnabled;
}

bool UOnsetGA_Generic::IsFriendlyActor(AOnsetBaseCharacter* Self, AOnsetBaseCharacter* HitChar) const
{
	return Self && HitChar && (HitChar == Self || !ShouldAffectActor(Self, HitChar));
}

AOnsetBaseCharacter* UOnsetGA_Generic::ResolveHealRecipient(AOnsetBaseCharacter* Self, AOnsetBaseCharacter* TargetChar) const
{
	if (!Self || !TargetChar || !TargetChar->AbilitySystemComponent)
	{
		return nullptr;
	}

	// Direct ally: heal the target itself.
	if (IsFriendlyActor(Self, TargetChar))
	{
		return TargetChar;
	}

	// Hostile target: Target-of-Target healing - heal whoever the enemy is attacking.
	if (TargetChar->TargetingComponent)
	{
		AActor* TargetOfTarget = TargetChar->TargetingComponent->GetTarget();
		AOnsetBaseCharacter* RecipientChar = Cast<AOnsetBaseCharacter>(TargetOfTarget);
		if (RecipientChar && RecipientChar->AbilitySystemComponent && IsFriendlyActor(Self, RecipientChar))
		{
			return RecipientChar;
		}
	}

	return nullptr;
}

void UOnsetGA_Generic::ResolveAbility(const FOnsetAbilityDefinition& Definition,
									  AOnsetBaseCharacter* Self,
									  AActor* TargetActor,
									  const FGameplayAbilitySpecHandle Handle,
									  const FGameplayAbilityActorInfo* ActorInfo,
									  const FGameplayAbilityActivationInfo ActivationInfo)
{
	switch (Definition.AbilityType)
	{
		case EOnsetAbilityType::SingleTarget:
		{
			AOnsetBaseCharacter* TargetChar = Cast<AOnsetBaseCharacter>(TargetActor);
			if (!TargetChar || !TargetChar->AbilitySystemComponent)
			{
				EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
				return;
			}

			// Hostile effects hit the aimed target; friendly effects (TT heal) hit the
			// resolved ally (the target itself, or the enemy's current target).
			AOnsetBaseCharacter* HealRecipient = ResolveHealRecipient(Self, TargetChar);

			// Play montage if available, delaying the hit to DamageTime.
			UAnimMontage* Montage = Definition.Montage.LoadSynchronous();
			if (Montage && Self->GetMesh() && Self->GetMesh()->GetAnimInstance())
			{
				const float MontageDuration = Self->PlayAnimMontage(Montage);
				if (MontageDuration > 0.0f)
				{
					CachedTargetASC = TargetChar->AbilitySystemComponent;
					CachedFriendlyASC = HealRecipient ? HealRecipient->AbilitySystemComponent : nullptr;

					FTimerDelegate TimerDelegate;
					TimerDelegate.BindUObject(this, &UOnsetGA_Generic::ApplyCachedDamageAfterDelay, Handle, ActorInfo, ActivationInfo);
					if (UWorld* World = Self->GetWorld())
					{
						World->GetTimerManager().SetTimer(MontageTimerHandle, TimerDelegate, Definition.DamageTime, false);
					}
					return;
				}
			}

			ApplyEffects(Definition, TargetChar->AbilitySystemComponent,
						 HealRecipient ? HealRecipient->AbilitySystemComponent : nullptr, GetAbilityLevel());
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
			break;
		}

		case EOnsetAbilityType::AoE:
		{
			// Target-centered (validated within CastRange + LoS at activation).
			const FVector Origin = IsValid(TargetActor) ? TargetActor->GetActorLocation() : Self->GetActorLocation();
			ApplyAoEAtLocation(Definition, Self, Origin);
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
			break;
		}

		case EOnsetAbilityType::PointBlankAoE:
		{
			// Self-centered; after a leap this is the landing spot.
			ApplyAoEAtLocation(Definition, Self, Self->GetActorLocation());
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
			break;
		}

		case EOnsetAbilityType::Self:
		{
			// Caster-targeted: friendly effects (heal/buff) apply to self; hostile
			// effects have no valid target and are skipped.
			if (!Self->AbilitySystemComponent)
			{
				EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
				return;
			}

			ApplyEffects(Definition, nullptr, Self->AbilitySystemComponent, GetAbilityLevel());
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
			break;
		}

		case EOnsetAbilityType::Cone:
		{
			UWorld* World = Self->GetWorld();
			if (!World)
			{
				EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
				return;
			}

			const FVector Start = Self->GetActorLocation();
			FVector FlatForward = Self->GetActorForwardVector();
			FlatForward.Z = 0.f;
			FlatForward.Normalize();

			TArray<AActor*> ActorsToIgnore;
			ActorsToIgnore.Add(Self);
			TArray<AActor*> OverlapActors;

			UKismetSystemLibrary::SphereOverlapActors(
				World,
				Start,
				Definition.Radius,
				TArray<TEnumAsByte<EObjectTypeQuery>>{UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel1)},
				nullptr,
				ActorsToIgnore,
				OverlapActors);

			const float ConeDotThreshold = FMath::Cos(FMath::DegreesToRadians(Definition.ConeHalfAngle));

			for (AActor* HitActor : OverlapActors)
			{
				AOnsetBaseCharacter* HitChar = Cast<AOnsetBaseCharacter>(HitActor);
				if (!IsValid(HitActor) || HitActor == Self || !HitChar)
				{
					continue;
				}

				FVector ToTarget = HitChar->GetActorLocation() - Start;
				ToTarget.Z = 0.f;
				ToTarget.Normalize();
				if (FVector::DotProduct(FlatForward, ToTarget) < ConeDotThreshold)
				{
					continue; // Outside the cone
				}

				ApplyEffectsToCharacter(Definition, Self, HitChar, GetAbilityLevel());
			}

			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
			break;
		}

		default:
			EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
			break;
	}
}

void UOnsetGA_Generic::ApplyAoEAtLocation(const FOnsetAbilityDefinition& Definition,
										  AOnsetBaseCharacter* Self,
										  const FVector& Origin)
{
	if (!Self)
	{
		return;
	}

	UWorld* World = Self->GetWorld();
	if (!World)
	{
		return;
	}

	TArray<FOverlapResult> OverlapResults;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(Definition.Radius);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Self);

	World->OverlapMultiByChannel(
		OverlapResults,
		Origin,
		FQuat::Identity,
		ECC_GameTraceChannel1,
		Sphere,
		QueryParams);

	for (const FOverlapResult& Result : OverlapResults)
	{
		AActor* HitActor = Result.GetActor();
		if (!IsValid(HitActor) || HitActor == Self)
		{
			continue;
		}

		AOnsetBaseCharacter* HitChar = Cast<AOnsetBaseCharacter>(HitActor);
		if (!HitChar)
		{
			continue;
		}

		ApplyEffectsToCharacter(Definition, Self, HitChar, GetAbilityLevel());
	}
}

void UOnsetGA_Generic::PerformLeap(const FOnsetAbilityDefinition& Definition,
								   AOnsetBaseCharacter* Self,
								   const FVector& Destination,
								   const FGameplayAbilitySpecHandle Handle,
								   const FGameplayAbilityActorInfo* ActorInfo,
								   const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (!Self)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	LeapStartLocation = Self->GetActorLocation();
	LeapDestinationLocation = Destination;
	LeapElapsedSeconds = 0.0f;
	LeapDurationSeconds = FMath::Max(0.05f, Definition.Movement.LeapDuration);
	LeapArcHeight = FMath::Max(0.0f, Definition.Movement.LeapArcHeight);

	// Freeze locomotion so movement doesn't fight the arc; restored on landing.
	if (UCharacterMovementComponent* MoveComp = Self->GetCharacterMovement())
	{
		SavedMovementMode = static_cast<uint8>(MoveComp->MovementMode);
		MoveComp->DisableMovement();
	}

	if (UWorld* World = Self->GetWorld())
	{
		World->GetTimerManager().SetTimer(LeapTimerHandle, this, &UOnsetGA_Generic::TickLeap, 1.0f / 30.0f, true);

		FTimerDelegate CompletionDelegate;
		CompletionDelegate.BindUObject(this, &UOnsetGA_Generic::OnLeapFinished, Handle, ActorInfo, ActivationInfo);
		World->GetTimerManager().SetTimer(LeapCompletionTimerHandle, CompletionDelegate, LeapDurationSeconds, false);
	}
}

void UOnsetGA_Generic::TickLeap()
{
	AOnsetBaseCharacter* Self = Cast<AOnsetBaseCharacter>(GetAvatarActorFromActorInfo());
	if (!Self)
	{
		return;
	}

	constexpr float TickInterval = 1.0f / 30.0f;
	LeapElapsedSeconds += TickInterval;

	const float Progress = FMath::Clamp(LeapDurationSeconds > 0.0f ? LeapElapsedSeconds / LeapDurationSeconds : 1.0f, 0.0f, 1.0f);

	// Parabolic arc: horizontal lerp plus a sine peak on Z.
	FVector Location = FMath::Lerp(LeapStartLocation, LeapDestinationLocation, Progress);
	Location.Z += FMath::Sin(Progress * UE_PI) * LeapArcHeight;

	Self->SetActorLocation(Location);

	// Face the landing spot while airborne.
	FVector ToDestination = LeapDestinationLocation - Self->GetActorLocation();
	ToDestination.Z = 0.0f;
	if (!ToDestination.IsNearlyZero())
	{
		Self->SetActorRotation(ToDestination.Rotation());
	}
}

void UOnsetGA_Generic::OnLeapFinished(const FGameplayAbilitySpecHandle Handle,
									  const FGameplayAbilityActorInfo* ActorInfo,
									  const FGameplayAbilityActivationInfo ActivationInfo)
{
	AOnsetBaseCharacter* Self = Cast<AOnsetBaseCharacter>(ActorInfo->AvatarActor);
	if (UWorld* World = Self ? Self->GetWorld() : nullptr)
	{
		World->GetTimerManager().ClearTimer(LeapTimerHandle);
		World->GetTimerManager().ClearTimer(LeapCompletionTimerHandle);
	}

	if (!Self)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	// Snap to the exact landing spot and restore locomotion.
	Self->SetActorLocation(LeapDestinationLocation);
	if (UCharacterMovementComponent* MoveComp = Self->GetCharacterMovement())
	{
		MoveComp->SetMovementMode(static_cast<EMovementMode>(SavedMovementMode));
	}

	const FOnsetAbilityDefinition* Definition = ResolveDefinition(Handle, ActorInfo);
	if (!Definition)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}
	CachedThreatMultiplier = FMath::Max(0.0f, Definition->ThreatMultiplier);

	AActor* TargetActor = Self->TargetingComponent ? Self->TargetingComponent->GetTarget() : nullptr;
	ResolveAbility(*Definition, Self, TargetActor, Handle, ActorInfo, ActivationInfo);
}

void UOnsetGA_Generic::ApplyCachedDamageAfterDelay(const FGameplayAbilitySpecHandle Handle,
												   const FGameplayAbilityActorInfo* ActorInfo,
												   const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (const FOnsetAbilityDefinition* Definition = ResolveDefinition(Handle, ActorInfo))
	{
		UAbilitySystemComponent* HostileASC = CachedTargetASC.Get();
		UAbilitySystemComponent* FriendlyASC = CachedFriendlyASC.Get();
		if (HostileASC || FriendlyASC)
		{
			ApplyEffects(*Definition, HostileASC, FriendlyASC, GetAbilityLevel());
		}
	}
	CachedTargetASC = nullptr;
	CachedFriendlyASC = nullptr;
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UOnsetGA_Generic::ApplyEffects(const FOnsetAbilityDefinition& Definition,
									UAbilitySystemComponent* HostileASC,
									UAbilitySystemComponent* FriendlyASC,
									float Level) const
{
	for (const FOnsetAbilityEffect& Effect : Definition.Effects)
	{
		UAbilitySystemComponent* RecipientASC = Effect.bFriendly ? FriendlyASC : HostileASC;
		if (RecipientASC)
		{
			ApplyEffect(Definition, Effect, RecipientASC, Level);
		}
	}
}

void UOnsetGA_Generic::ApplyEffectsToCharacter(const FOnsetAbilityDefinition& Definition,
											   AOnsetBaseCharacter* Self,
											   AOnsetBaseCharacter* HitChar,
											   float Level) const
{
	if (!Self || !HitChar || !HitChar->AbilitySystemComponent)
	{
		return;
	}

	// Hostile effects land on enemies, friendly effects on allies (no friendly fire).
	const bool bFriendly = IsFriendlyActor(Self, HitChar);
	ApplyEffects(Definition, bFriendly ? nullptr : HitChar->AbilitySystemComponent,
				 bFriendly ? HitChar->AbilitySystemComponent : nullptr, Level);
}

void UOnsetGA_Generic::ApplyEffect(const FOnsetAbilityDefinition& Definition,
								   const FOnsetAbilityEffect& Effect,
								   UAbilitySystemComponent* TargetASC,
								   float Level) const
{
	switch (Effect.Type)
	{
		case EOnsetAbilityEffectType::Damage:
		{
			const FGameplayTag ElementTag = Effect.DamageTypeTag.IsValid() ? Effect.DamageTypeTag : TAG_Damage_Physical;

			if (Effect.Period > 0.0f)
			{
				// Damage-over-time: raw = SourceStat × DoTCoefficient (Magnitude), where
				// SourceStat = STR for Physical, INT for elemental (combat-formulas §8).
				const UOnsetCombatAttributeSet* SourceCombat = GetSourceCombatAttributes();
				const float SourceStat = SourceCombat
					? ((ElementTag == TAG_Damage_Physical) ? SourceCombat->GetStrength() : SourceCombat->GetIntellect())
					: 0.0f;
				const float TickRaw = SourceStat * Effect.Magnitude;

				TMap<FGameplayTag, float> TagMagnitudes;
				TagMagnitudes.Add(ElementTag, TickRaw);
				TMap<FName, float> NameMagnitudes;
				NameMagnitudes.Add(TEXT("Duration"), Effect.Duration);
				ApplyPeriodicEffectSpecToTarget(UOnsetGenericDamageOverTimeEffect::StaticClass(),
												TargetASC, NameMagnitudes, TagMagnitudes, Effect.Duration, Effect.Period,
												Definition.RefreshTag, Level);
				break;
			}

			// Weapon-scaled effects use the equipped weapon's WeaponBase; skill-scaled
			// effects use the row's Magnitude as SkillBase.
			const float Base = (Effect.ScalingType == EOnsetScalingType::Weapon)
				? GetSourceWeaponBase()
				: Effect.Magnitude;
			const float Raw = ResolveScaledBase(Base, Effect.ScalingType);
			ApplyDamageToTarget(TargetASC, ElementTag, Raw, Level);
			break;
		}

		case EOnsetAbilityEffectType::Heal:
		{
			TMap<FName, float> Magnitudes;
			// Support mastery: EffectiveBuffValue = Base x (1 + Potency) for buff/debuff magnitudes.
			Magnitudes.Add(TEXT("HealAmount"), Effect.Magnitude * GetBuffPotency());

			if (Effect.Period > 0.0f)
			{
				// Heal-over-time: ticks every Period seconds for the Duration window.
				Magnitudes.Add(TEXT("Duration"), Effect.Duration);
				ApplyPeriodicEffectSpecToTarget(UOnsetGenericHealOverTimeEffect::StaticClass(),
												TargetASC, Magnitudes, {}, Effect.Duration, Effect.Period,
												Definition.RefreshTag, Level);
				break;
			}

			ApplyEffectSpecToTarget(UOnsetGenericHealEffect::StaticClass(), TargetASC, Magnitudes, Level);
			break;
		}

		case EOnsetAbilityEffectType::Snare:
		{
			TMap<FName, float> Magnitudes;
			Magnitudes.Add(TEXT("MoveSpeedMod"), Effect.Magnitude * GetBuffPotency());
			Magnitudes.Add(TEXT("Duration"), Effect.Duration);
			ApplyEffectSpecToTarget(UOnsetGenericSnareEffect::StaticClass(), TargetASC, Magnitudes, Level);
			break;
		}

		case EOnsetAbilityEffectType::Slow:
		{
			TMap<FName, float> Magnitudes;
			Magnitudes.Add(TEXT("CooldownRateMod"), Effect.Magnitude * GetBuffPotency());
			Magnitudes.Add(TEXT("Duration"), Effect.Duration);
			ApplyEffectSpecToTarget(UOnsetCooldownSlowEffect::StaticClass(), TargetASC, Magnitudes, Level);
			break;
		}

		case EOnsetAbilityEffectType::Stun:
		{
			// CC diminishing returns: 100% → 50% → 25% → immune for consecutive applications.
			const float EffectiveDuration = GetDiminishedCCDuration(TargetASC, TAG_State_Stunned, Effect.Duration);
			if (EffectiveDuration > 0.0f)
			{
				TMap<FName, float> Magnitudes;
				Magnitudes.Add(TEXT("Duration"), EffectiveDuration);
				ApplyEffectSpecToTarget(UOnsetGenericStunEffect::StaticClass(), TargetASC, Magnitudes, Level);
			}
			break;
		}

		case EOnsetAbilityEffectType::Freeze:
		{
			const float EffectiveDuration = GetDiminishedCCDuration(TargetASC, TAG_State_Frozen, Effect.Duration);
			if (EffectiveDuration > 0.0f)
			{
				TMap<FName, float> Magnitudes;
				Magnitudes.Add(TEXT("Duration"), EffectiveDuration);
				ApplyEffectSpecToTarget(UOnsetGenericFreezeEffect::StaticClass(), TargetASC, Magnitudes, Level);
			}
			break;
		}

		case EOnsetAbilityEffectType::Invulnerable:
		{
			TMap<FName, float> Magnitudes;
			Magnitudes.Add(TEXT("Duration"), Effect.Duration);
			ApplyEffectSpecToTarget(UOnsetGenericInvulnerableEffect::StaticClass(), TargetASC, Magnitudes, Level);
			break;
		}

		default:
			break;
	}
}

void UOnsetGA_Generic::ApplyEffectSpecToTarget(TSubclassOf<UGameplayEffect> EffectClass,
											   UAbilitySystemComponent* TargetASC,
											   const TMap<FName, float>& SetByCallerMagnitudes,
											   float Level) const
{
	if (!EffectClass || !TargetASC)
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
	// Carry the ability on the context so UOnsetAttributeSet can resolve its
	// threat multiplier when this effect's damage generates threat.
	Context.SetAbility(this);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, Level, Context);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	for (const TPair<FName, float>& Pair : SetByCallerMagnitudes)
	{
		SpecHandle.Data->SetSetByCallerMagnitude(Pair.Key, Pair.Value);
	}

	TargetASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

float UOnsetGA_Generic::GetDiminishedCCDuration(UAbilitySystemComponent* TargetASC,
												FGameplayTag CCType,
												float BaseDuration) const
{
	const AOnsetBaseCharacter* Target = TargetASC ? Cast<AOnsetBaseCharacter>(TargetASC->GetOwnerActor()) : nullptr;
	if (!Target || !Target->CCDiminishing)
	{
		return BaseDuration;
	}
	return Target->CCDiminishing->GetDiminishedDuration(CCType, BaseDuration);
}

void UOnsetGA_Generic::ApplyPeriodicEffectSpecToTarget(TSubclassOf<UGameplayEffect> EffectClass,
													   UAbilitySystemComponent* TargetASC,
													   const TMap<FName, float>& NameMagnitudes,
													   const TMap<FGameplayTag, float>& TagMagnitudes,
													   float Duration,
													   float Period,
													   FGameplayTag InRefreshTag,
													   float Level) const
{
	if (!EffectClass || !TargetASC || Period <= 0.0f || Duration <= 0.0f)
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
	// Carry the ability on the context so UOnsetAttributeSet can resolve its
	// threat multiplier for each DoT tick's generated threat.
	Context.SetAbility(this);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, Level, Context);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	for (const TPair<FName, float>& Pair : NameMagnitudes)
	{
		SpecHandle.Data->SetSetByCallerMagnitude(Pair.Key, Pair.Value);
	}
	for (const TPair<FGameplayTag, float>& Pair : TagMagnitudes)
	{
		SpecHandle.Data->SetSetByCallerMagnitude(Pair.Key, Pair.Value);
	}

	// The engine reads Duration and Period from the spec, so override both from the row.
	SpecHandle.Data->SetDuration(Duration, true);
	SpecHandle.Data->Period = Period;

	// Grant the row's RefreshTag on the active effect so consumers can query the target
	// for an existing instance (AI selection refresh gate: re-DoT scores 0 while active).
	if (InRefreshTag.IsValid())
	{
		SpecHandle.Data->DynamicGrantedTags.AddTag(InRefreshTag);
	}

	TargetASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

float UOnsetGA_Generic::GetComparisonDamage(const FOnsetAbilityDefinition& Definition) const
{
	const UOnsetCombatAttributeSet* Combat = GetSourceCombatAttributes();
	const float Strength = Combat ? Combat->GetStrength() : 0.0f;
	const float Intellect = Combat ? Combat->GetIntellect() : 0.0f;
	const float WeaponBase = GetSourceWeaponBase();

	// Snapshot invalidation: any drift in the inputs the damage math depends on
	// (stat buffs/debuffs, equipment swaps) triggers a recompute. Two/three float
	// compares per selection query; recomputes are rare.
	if (!bComparisonDamageValid
		|| !FMath::IsNearlyEqual(Strength, ComparisonStrengthSnapshot)
		|| !FMath::IsNearlyEqual(Intellect, ComparisonIntellectSnapshot)
		|| !FMath::IsNearlyEqual(WeaponBase, ComparisonWeaponBaseSnapshot))
	{
		CachedComparisonDamage = ComputeComparisonDamageInternal(Definition, Strength, Intellect, WeaponBase);
		ComparisonStrengthSnapshot = Strength;
		ComparisonIntellectSnapshot = Intellect;
		ComparisonWeaponBaseSnapshot = WeaponBase;
		bComparisonDamageValid = true;
	}
	return CachedComparisonDamage;
}

float UOnsetGA_Generic::ComputeComparisonDamageInternal(const FOnsetAbilityDefinition& Definition,
														float Strength, float Intellect, float WeaponBase) const
{
	// Per-target expected damage, mirroring ApplyEffect exactly:
	// - Direct: Base x (1 + Stat/100), Base = WeaponBase (Weapon scaling) | Magnitude (Skill).
	// - DoT:    (STR for Physical | INT for elemental) x Magnitude per tick,
	//           Duration / Period ticks over the window.
	// Non-damage effects contribute 0 (utility scoring is a future pass).
	float Total = 0.0f;
	for (const FOnsetAbilityEffect& Effect : Definition.Effects)
	{
		if (Effect.Type != EOnsetAbilityEffectType::Damage)
		{
			continue;
		}

		const bool bPhysical = !Effect.DamageTypeTag.IsValid()
			|| Effect.DamageTypeTag == TAG_Damage_Physical;

		if (Effect.Period > 0.0f)
		{
			const float SourceStat = bPhysical ? Strength : Intellect;
			const int32 TickCount = FMath::Max(1, FMath::FloorToInt32(Effect.Duration / FMath::Max(Effect.Period, KINDA_SMALL_NUMBER)));
			Total += SourceStat * Effect.Magnitude * TickCount;
		}
		else
		{
			const float Base = (Effect.ScalingType == EOnsetScalingType::Weapon) ? WeaponBase : Effect.Magnitude;
			const float StatValue = (Effect.ScalingType == EOnsetScalingType::Weapon) ? Strength : Intellect;
			Total += Base * (1.0f + StatValue / 100.0f);
		}
	}
	return Total;
}

bool UOnsetGA_Generic::HasActivePeriodicInstanceFrom(const UAbilitySystemComponent* TargetASC,
													 FGameplayTag RefreshTag,
													 const AActor* SourceAvatar)
{
	if (!TargetASC || !RefreshTag.IsValid() || !SourceAvatar)
	{
		return false;
	}

	// Scan active effects for one carrying our dynamic RefreshTag AND instigated by
	// this specific caster. Tag-only matching would gate every player behind the first
	// applier's stack, which is wrong under per-source DoT stacking.
	for (const FActiveGameplayEffectHandle& Handle : TargetASC->GetActiveEffects(FGameplayEffectQuery()))
	{
		const FActiveGameplayEffect* Active = TargetASC->GetActiveGameplayEffect(Handle);
		if (!Active || !Active->Spec.DynamicGrantedTags.HasTag(RefreshTag))
		{
			continue;
		}
		if (Active->Spec.GetContext().GetInstigator() == SourceAvatar)
		{
			return true;
		}
	}
	return false;
}

void UOnsetGA_Generic::ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
									 const FGameplayAbilityActorInfo* ActorInfo,
									 const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const FOnsetAbilityDefinition* Definition = ResolveDefinition(Handle, ActorInfo);
	if (!Definition || !ActorInfo)
	{
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(UOnsetGenericCooldownEffect::StaticClass(), GetAbilityLevel(Handle, ActorInfo), Context);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	// Add the per-ability cooldown tag dynamically (Cooldown.<RowName>).
	if (Definition->CooldownTag.IsValid())
	{
		SpecHandle.Data->DynamicGrantedTags.AddTag(Definition->CooldownTag);
	}

	// Scale the cooldown by the source's CooldownMultiplier (Slow debuff).
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
	Multiplier = FMath::Max(0.1f, Multiplier);

	// Haste/CDR shortens the row base: EffectiveCooldown = CooldownSeconds x (1 - TotalCDR%) x Multiplier.
	const float FinalDuration = FMath::Max(0.1f,
		Definition->CooldownSeconds * (1.0f - GetTotalCooldownReduction()) * Multiplier);
	SpecHandle.Data->SetDuration(FinalDuration, true);

	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}
