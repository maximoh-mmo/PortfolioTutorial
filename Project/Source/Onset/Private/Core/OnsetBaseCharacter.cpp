// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/OnsetBaseCharacter.h"

#include "AbilitySystemComponent.h"
#include "GAS/OnsetAttributeSet.h"
#include "Combat/OnsetGA_BasicAttack.h"
#include "Combat/OnsetGA_HitReaction.h"
#include "Combat/OnsetGA_AoE.h"
#include "Combat/OnsetGA_Cone.h"
#include "Combat/OnsetGA_Shadowstep.h"
#include "Combat/OnsetGA_CooldownSlow.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/OnsetMovementAttributeSet.h"
#include "GAS/OnsetCombatAttributeSet.h"
#include "Materials/MaterialInterface.h"
#include "Core/TargetingComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AOnsetBaseCharacter::AOnsetBaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	TargetingComponent = CreateDefaultSubobject<UTargetingComponent>(TEXT("TargetingComponent"));
	AttributeSet = CreateDefaultSubobject<UOnsetAttributeSet>(TEXT("AttributeSet"));
	MovementAttributes = CreateDefaultSubobject<UOnsetMovementAttributeSet>(TEXT("MovementAttributes"));
	CombatAttributes = CreateDefaultSubobject<UOnsetCombatAttributeSet>(TEXT("CombatAttributes"));

	// Ground reticle decal: hidden until this character becomes the player's target.
	TargetReticleDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("TargetReticleDecal"));
	TargetReticleDecal->SetupAttachment(GetCapsuleComponent());
	TargetReticleDecal->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	TargetReticleDecal->SetRelativeLocation(FVector(0.0f, 0.0f, -(GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 60.0f)));
	TargetReticleDecal->DecalSize = FVector(120.0f, 50.0f, 50.0f);
	TargetReticleDecal->SetHiddenInGame(true);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultReticleMat(TEXT("/Game/Materials/M_TargetReticle.M_TargetReticle"));
	if (DefaultReticleMat.Succeeded())
	{
		TargetReticleMaterial = DefaultReticleMat.Object;
	}
	else
	{
		// Fallback so the reticle still renders if the content asset is missing.
		static ConstructorHelpers::FObjectFinder<UMaterialInterface> FallbackReticleMat(TEXT("/Engine/EngineDebugMaterials/DefaultDeferredDecalMaterial.DefaultDeferredDecalMaterial"));
		if (FallbackReticleMat.Succeeded())
		{
			TargetReticleMaterial = FallbackReticleMat.Object;
		}
	}
}

void AOnsetBaseCharacter::InitAbilityActorInfo()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void AOnsetBaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (!HasAuthority()) return;
	InitAbilityActorInfo();
	GrantDefaultAbilities();
}

void AOnsetBaseCharacter::GrantDefaultAbilities()
{
	if (!AbilitySystemComponent || bAbilitiesGranted) return;

	UClass* BasicAttackAbility = LoadObject<UClass>(nullptr, (TEXT("/Game/Game/Combat/GA_BasicAttack.GA_BasicAttack_C")));
	if (!BasicAttackAbility) BasicAttackAbility = UOnsetGA_BasicAttack::StaticClass();
	AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(BasicAttackAbility, 1, INDEX_NONE, this));

	UClass* HitReaction = LoadObject<UClass>(nullptr, (TEXT("/Game/Game/Combat/GA_HitReaction.GA_HitReaction_C")));
	if (!HitReaction) HitReaction = UOnsetGA_HitReaction::StaticClass();
	AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(HitReaction, 1, INDEX_NONE, this));

	UClass* AoEAbility = LoadObject<UClass>(nullptr, (TEXT("/Game/Game/Combat/GA_AoE.GA_AoE_C")));
	if (!AoEAbility) AoEAbility = UOnsetGA_AoE::StaticClass();
	AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AoEAbility, 1, 1, this)); // Input ID 1

	UClass* ConeAbility = LoadObject<UClass>(nullptr, (TEXT("/Game/Game/Combat/GA_Cone.GA_Cone_C")));
	if (!ConeAbility) ConeAbility = UOnsetGA_Cone::StaticClass();
	AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(ConeAbility, 1, 2, this)); // Input ID 2

	UClass* ShadowstepAbility = LoadObject<UClass>(nullptr, (TEXT("/Game/Game/Combat/GA_Shadowstep.GA_Shadowstep_C")));
	if (!ShadowstepAbility) ShadowstepAbility = UOnsetGA_Shadowstep::StaticClass();
	AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(ShadowstepAbility, 1, INDEX_NONE, this)); // Passive

	UClass* CooldownSlowAbility = LoadObject<UClass>(nullptr, (TEXT("/Game/Game/Combat/GA_CooldownSlow.GA_CooldownSlow_C")));
	if (!CooldownSlowAbility) CooldownSlowAbility = UOnsetGA_CooldownSlow::StaticClass();
	AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(CooldownSlowAbility, 1, 3, this)); // Input ID 3

	bAbilitiesGranted = true;
}

void AOnsetBaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AOnsetBaseCharacter, bIsAlive, COND_None);
}

void AOnsetBaseCharacter::OnRep_bIsAlive()
{
	SetActorHiddenInGame(!bIsAlive);
	SetActorEnableCollision(bIsAlive);
}

void AOnsetBaseCharacter::ResetAttributes()
{
	if (!AbilitySystemComponent) return;
	AttributeSet->InitHealth(AttributeSet->GetMaxHealth());
	AttributeSet->InitMaxHealth(100.0f);
	MovementAttributes->InitMovementSpeed(600.0f);
	CombatAttributes->InitCooldownMultiplier(1.0f);
	bIsAlive = false; // Pool return — OnRespawn() re-enables on retrieval
}

void AOnsetBaseCharacter::OnDeath(AActor* KillingActor)
{
	bIsAlive = false;
	SetTargetReticle(false);
}

bool AOnsetBaseCharacter::IsAlive() const
{
	return bIsAlive && AttributeSet && AttributeSet->GetHealth() > 0.0f;
}

void AOnsetBaseCharacter::OnRespawn()
{
	bIsAlive = true;
	SetActorHiddenInGame(false);                                                                   
	SetActorTickEnabled(true);
	SetActorEnableCollision(true);
}

void AOnsetBaseCharacter::SetTargetReticle(bool bShow)
{
	if (!TargetReticleDecal)
	{
		return;
	}

	bTargetReticleVisible = bShow;

	if (bShow)
	{
		if (TargetReticleMaterial)
		{
			TargetReticleDecal->SetDecalMaterial(TargetReticleMaterial);
		}

		UpdateTargetReticle();
		TargetReticleDecal->SetHiddenInGame(false);
	}
	else
	{
		TargetReticleDecal->SetHiddenInGame(true);
	}
}

void AOnsetBaseCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bTargetReticleVisible)
	{
		UpdateTargetReticle();
	}
}

void AOnsetBaseCharacter::UpdateTargetReticle()
{
	if (!TargetReticleDecal || !GetWorld())
	{
		return;
	}

	// Trace straight down from the capsule center to find the surface below,
	// then park the decal there so it hugs uneven terrain. The projection box
	// is centered on the decal (DecalSize is a half-extent), so sink it by the
	// full depth: the top face sits at the surface and it never reaches back
	// up into the character's body.
	const FVector TraceStart = GetCapsuleComponent()->GetComponentLocation();
	const FVector TraceEnd = TraceStart - FVector(0.0f, 0.0f, 2000.0f);

	FHitResult Hit;
	FCollisionQueryParams QueryParams(FName(TEXT("TargetReticleTrace")), false, this);
	QueryParams.bTraceComplex = false;

	if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams) && Hit.bBlockingHit)
	{
		const float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
		constexpr float ReticleDepth = 80.0f;

		// Decals project along local +X, so point +X against the surface normal.
		const FRotator SurfaceRotation = FRotationMatrix::MakeFromX(-Hit.ImpactNormal).Rotator();

		// Slight margin above the surface so the ground plane is strictly inside the box.
		const FVector ReticleCenter = Hit.ImpactPoint - Hit.ImpactNormal * (ReticleDepth - 5.0f);
		TargetReticleDecal->SetWorldLocationAndRotation(ReticleCenter, SurfaceRotation);
		TargetReticleDecal->DecalSize = FVector(ReticleDepth, Radius * 1.4f, Radius * 1.4f);
	}
}
