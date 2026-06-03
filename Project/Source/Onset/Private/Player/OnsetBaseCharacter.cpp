// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/OnsetBaseCharacter.h"

#include "AbilitySystemComponent.h"
#include "Combat/OnsetAttributeSet.h"
#include "Combat/GameplayAbilities/OnsetGA_BasicAttack.h"
#include "Player/TargetingComponent.h"

AOnsetBaseCharacter::AOnsetBaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	TargetingComponent = CreateDefaultSubobject<UTargetingComponent>(TEXT("TargetingComponent"));
	AttributeSet = CreateDefaultSubobject<UOnsetAttributeSet>(TEXT("AttributeSet"));
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
	FGameplayAbilitySpec Spec(BasicAttackAbility, 1, INDEX_NONE, this);
	AbilitySystemComponent->GiveAbility(Spec);
	
	bAbilitiesGranted = true;
}
