// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/OnsetBaseCharacter.h"

#include "AbilitySystemComponent.h"
#include "Combat/OnsetAttributeSet.h"

AOnsetBaseCharacter::AOnsetBaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
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
}
