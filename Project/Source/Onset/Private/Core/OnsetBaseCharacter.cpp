// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/OnsetBaseCharacter.h"

#include "AbilitySystemComponent.h"
#include "GAS/OnsetAttributeSet.h"
#include "Combat/OnsetGA_BasicAttack.h"
#include "Combat/OnsetGA_HitReaction.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/OnsetMovementAttributeSet.h"
#include "Core/TargetingComponent.h"

AOnsetBaseCharacter::AOnsetBaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
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
	
	bAbilitiesGranted = true;
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
