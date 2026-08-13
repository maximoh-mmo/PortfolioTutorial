// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/OnsetGA_Generic.h"

#include "AbilitySystemComponent.h"
#include "Core/OnsetBaseCharacter.h"
#include "Core/TargetingComponent.h"
#include "Data/OnsetAbilityTypes.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GAS/OnsetCombatAttributeSet.h"
#include "GAS/OnsetCooldownSlowEffect.h"
#include "GAS/OnsetGenericCooldownEffect.h"
#include "GAS/OnsetGenericDamageEffect.h"
#include "GAS/OnsetGenericSnareEffect.h"
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

			return HasLineOfSight(Self, TargetActor);
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
			// Self-centered: no target, range, or LoS gate.
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
	if (!Self || !HitChar)
	{
		return false;
	}

	// PvP filtering: skip players unless both sides have PvP enabled.
	if (HitChar->IsA(AOnsetPlayerCharacter::StaticClass()))
	{
		AOnsetPlayerState* SelfPS = Self->GetPlayerState<AOnsetPlayerState>();
		AOnsetPlayerState* TargetPS = HitChar->GetPlayerState<AOnsetPlayerState>();
		if (SelfPS && TargetPS)
		{
			if (!SelfPS->bIsPvPEnabled || !TargetPS->bIsPvPEnabled)
			{
				return false;
			}
		}
	}

	return true;
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

			// Play montage if available, delaying the hit to DamageTime.
			UAnimMontage* Montage = Definition.Montage.LoadSynchronous();
			if (Montage && Self->GetMesh() && Self->GetMesh()->GetAnimInstance())
			{
				const float MontageDuration = Self->PlayAnimMontage(Montage);
				if (MontageDuration > 0.0f)
				{
					CachedTargetASC = TargetChar->AbilitySystemComponent;

					FTimerDelegate TimerDelegate;
					TimerDelegate.BindUObject(this, &UOnsetGA_Generic::ApplyCachedDamageAfterDelay, Handle, ActorInfo, ActivationInfo);
					if (UWorld* World = Self->GetWorld())
					{
						World->GetTimerManager().SetTimer(MontageTimerHandle, TimerDelegate, Definition.DamageTime, false);
					}
					return;
				}
			}

			ApplyEffects(Definition, TargetChar->AbilitySystemComponent, GetAbilityLevel());
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
				Definition.ConeRange,
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

				if (!ShouldAffectActor(Self, HitChar))
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

				if (HitChar->AbilitySystemComponent)
				{
					ApplyEffects(Definition, HitChar->AbilitySystemComponent, GetAbilityLevel());
				}
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
		if (!HitChar || !ShouldAffectActor(Self, HitChar))
		{
			continue;
		}

		if (HitChar->AbilitySystemComponent)
		{
			ApplyEffects(Definition, HitChar->AbilitySystemComponent, GetAbilityLevel());
		}
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

	AActor* TargetActor = Self->TargetingComponent ? Self->TargetingComponent->GetTarget() : nullptr;
	ResolveAbility(*Definition, Self, TargetActor, Handle, ActorInfo, ActivationInfo);
}

void UOnsetGA_Generic::ApplyCachedDamageAfterDelay(const FGameplayAbilitySpecHandle Handle,
												   const FGameplayAbilityActorInfo* ActorInfo,
												   const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (UAbilitySystemComponent* TargetASC = CachedTargetASC.Get())
	{
		if (const FOnsetAbilityDefinition* Definition = ResolveDefinition(Handle, ActorInfo))
		{
			ApplyEffects(*Definition, TargetASC, GetAbilityLevel());
		}
	}
	CachedTargetASC = nullptr;
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UOnsetGA_Generic::ApplyEffects(const FOnsetAbilityDefinition& Definition,
									UAbilitySystemComponent* TargetASC,
									float Level) const
{
	for (const FOnsetAbilityEffect& Effect : Definition.Effects)
	{
		ApplyEffect(Effect, TargetASC, Level);
	}
}

void UOnsetGA_Generic::ApplyEffect(const FOnsetAbilityEffect& Effect,
								   UAbilitySystemComponent* TargetASC,
								   float Level) const
{
	switch (Effect.Type)
	{
		case EOnsetAbilityEffectType::Damage:
		{
			const bool bMagical = (Effect.DamageTypeTag == TAG_Damage_Magical);
			ApplyDamageToTarget(TargetASC, bMagical ? 0.0f : Effect.Magnitude, bMagical ? Effect.Magnitude : 0.0f, Level);
			break;
		}

		case EOnsetAbilityEffectType::Snare:
		{
			TMap<FName, float> Magnitudes;
			Magnitudes.Add(TEXT("MoveSpeedMod"), Effect.Magnitude);
			Magnitudes.Add(TEXT("Duration"), Effect.Duration);
			ApplyEffectSpecToTarget(UOnsetGenericSnareEffect::StaticClass(), TargetASC, Magnitudes, Level);
			break;
		}

		case EOnsetAbilityEffectType::Slow:
		{
			TMap<FName, float> Magnitudes;
			Magnitudes.Add(TEXT("CooldownRateMod"), Effect.Magnitude);
			Magnitudes.Add(TEXT("Duration"), Effect.Duration);
			ApplyEffectSpecToTarget(UOnsetCooldownSlowEffect::StaticClass(), TargetASC, Magnitudes, Level);
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
	const float FinalDuration = FMath::Max(0.1f, Definition->CooldownSeconds * Multiplier);
	SpecHandle.Data->SetDuration(FinalDuration, true);

	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}
