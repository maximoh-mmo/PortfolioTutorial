// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/TargetingComponent.h"

#include "GameFramework/Actor.h"
#include "Player/OnsetPlayerCharacter.h"
#include "Player/OnsetPlayerController.h"
#include "Player/OnsetPlayerState.h"

UTargetingComponent::UTargetingComponent() : CurrentTarget(nullptr)
{
}

void UTargetingComponent::SetTarget(AActor* NewTarget)
{
	if (!IsActorTargetValid(NewTarget)) return;
	CurrentTarget = NewTarget;
}

void UTargetingComponent::ClearTarget()
{
	CurrentTarget = nullptr;
}

bool UTargetingComponent::IsActorTargetValid(AActor* Actor)
{
	if (!Actor) return false;
	if (const AOnsetBaseCharacter* Character = Cast<AOnsetBaseCharacter>(Actor); !Character) return false;
	return Actor != GetOwner();
}

// ReSharper disable once CppUE4CodingStandardNamingViolationWarning
bool UTargetingComponent::IsActorTargetPVPValid(AActor* TargetActor, AActor* SourceActor)
{
	if (const AOnsetPlayerCharacter* TargetCharacter = Cast<AOnsetPlayerCharacter>(TargetActor); !TargetCharacter) return false;
	if (const AOnsetPlayerController* SourceController = Cast<AOnsetPlayerController>(SourceActor->GetInstigatorController()))
	{
		AOnsetPlayerState* SourcePlayerState = SourceController->GetPlayerState<AOnsetPlayerState>();
		if (SourcePlayerState && !SourcePlayerState->bIsPvPEnabled)	return false;
	}
	return true;
}

