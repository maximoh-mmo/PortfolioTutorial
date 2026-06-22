// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/OnsetMovementAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Core/OnsetBaseCharacter.h"

UOnsetMovementAttributeSet::UOnsetMovementAttributeSet()
{
	InitMovementSpeed(600.0f);
}

void UOnsetMovementAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue,
	float NewValue)
{
	if (Attribute == GetMovementSpeedAttribute())
	{
		float ClampedSpeed = FMath::Max(NewValue, 0.0f);
		if (AOnsetBaseCharacter* Character = Cast<AOnsetBaseCharacter>(GetOwningActor()))
		{
			Character->GetCharacterMovement()->MaxWalkSpeed = ClampedSpeed;
		}
	}
}


void UOnsetMovementAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UOnsetMovementAttributeSet, MovementSpeed, COND_None, REPNOTIFY_Always);
}

void UOnsetMovementAttributeSet::OnRep_MovementSpeed(const FGameplayAttributeData& OldMovementSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOnsetMovementAttributeSet, MovementSpeed, OldMovementSpeed);
}
