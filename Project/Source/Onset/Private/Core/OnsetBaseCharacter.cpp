// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/OnsetBaseCharacter.h"

#include "AbilitySystemComponent.h"
#include "GAS/OnsetAttributeSet.h"
#include "Combat/OnsetGA_BasicAttack.h"
#include "Combat/OnsetGA_HitReaction.h"
#include "Combat/OnsetGA_AoE.h"
#include "Combat/OnsetGA_Cone.h"
#include "Combat/OnsetGA_Shadowstep.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/OnsetMovementAttributeSet.h"
#include "Core/TargetingComponent.h"
#include "Net/UnrealNetwork.h"

AOnsetBaseCharacter::AOnsetBaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	TargetingComponent = CreateDefaultSubobject<UTargetingComponent>(TEXT("TargetingComponent"));
	AttributeSet = CreateDefaultSubobject<UOnsetAttributeSet>(TEXT("AttributeSet"));
	MovementAttributes = CreateDefaultSubobject<UOnsetMovementAttributeSet>(TEXT("MovementAttributes"));
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
	bIsAlive = false; // Pool return — OnRespawn() re-enables on retrieval
}

void AOnsetBaseCharacter::OnDeath(AActor* KillingActor)
{
	bIsAlive = false;	
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
