// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/OnsetGA_Cone.h"

#include "AbilitySystemComponent.h"
#include "GAS/OnsetGameplayTags.h"
#include "Core/OnsetBaseCharacter.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "Player/OnsetPlayerCharacter.h"
#include "Player/OnsetPlayerState.h"

UOnsetGA_Cone::UOnsetGA_Cone()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(TAG_Ability_Cone);
	SetAssetTags(AssetTags);
}

void UOnsetGA_Cone::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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
	if (!Self)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	UWorld* World = Self->GetWorld();
	if (!World)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	// Cone overlap from character forward vector
	FVector Start = Self->GetActorLocation();
	FVector FlatForward = Self->GetActorForwardVector();
	FlatForward.Z = 0.f;
	FlatForward.Normalize();
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(Self);

	TArray<AActor*> OverlapActors;
	
	UKismetSystemLibrary::SphereOverlapActors(
		World, 
		Start, 
		ConeRange, 
		TArray<TEnumAsByte<EObjectTypeQuery>>{UEngineTypes::ConvertToObjectType(OverlapChannel)},
		nullptr,
		ActorsToIgnore,
		OverlapActors
		);

	if (OverlapActors.Num() == 0)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	
	const float ConeDotThreshold = FMath::Cos(FMath::DegreesToRadians(ConeHalfAngle));
	// Apply damage to each valid target with PvP filtering
	for (AActor* HitActor : OverlapActors)
	{		
		AOnsetBaseCharacter* HitChar = Cast<AOnsetBaseCharacter>(HitActor);
		if (!IsValid(HitActor) || HitActor == Self || !HitChar)
		{
			continue;
		}

		// PvP filtering: skip players if PvP disabled
		if (HitChar && HitChar->IsA(AOnsetPlayerCharacter::StaticClass()))
		{
			AOnsetPlayerState* SelfPS = Self->GetPlayerState<AOnsetPlayerState>();
			AOnsetPlayerState* TargetPS = HitChar->GetPlayerState<AOnsetPlayerState>();
			if (SelfPS && TargetPS)
			{
				if (!SelfPS->bIsPvPEnabled || !TargetPS->bIsPvPEnabled)
				{
					continue;
				}
			}
		}
		
		// Sphere trace for actors catches all in the sphere, we want those within the Cone Radius, use dot product to validate.
		
		FVector ToTarget = HitChar->GetActorLocation() - Start;
		ToTarget.Z = 0.f;
		ToTarget.Normalize();
		
		if (FVector::DotProduct(FlatForward, ToTarget) < ConeDotThreshold)
		{
			continue; // Outside the cone
		}

		if (HitChar->AbilitySystemComponent)
		{
			ApplyDamageToTarget(HitChar->AbilitySystemComponent, TAG_Damage_Physical, Damage, GetAbilityLevel());
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}