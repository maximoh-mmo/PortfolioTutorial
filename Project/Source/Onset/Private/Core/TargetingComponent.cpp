// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/TargetingComponent.h"


#include "Player/OnsetPlayerCharacter.h"
#include "Player/OnsetPlayerController.h"
#include "Player/OnsetPlayerState.h"
#include "Net/UnrealNetwork.h"

UTargetingComponent::UTargetingComponent() : CurrentTarget(nullptr)
{
	SetIsReplicatedByDefault(true);
}

void UTargetingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UTargetingComponent, CurrentTarget);
}

void UTargetingComponent::OnRep_CurrentTarget()
{
	OnTargetChanged.Broadcast(CurrentTarget);
}

void UTargetingComponent::SetTarget(AActor* NewTarget)
{
	if (!NewTarget)
	{
		ClearTarget();
		return;
	}
	if (!IsActorTargetValid(NewTarget)) return;
	CurrentTarget = NewTarget;
	OnTargetChanged.Broadcast(CurrentTarget);
}

void UTargetingComponent::ClearTarget()
{
	CurrentTarget = nullptr;
	OnTargetChanged.Broadcast(CurrentTarget);
}

bool UTargetingComponent::IsActorTargetValid(AActor* Actor) const
{
	if (!Actor) return false;
	if (const AOnsetBaseCharacter* Character = Cast<AOnsetBaseCharacter>(Actor); !Character) return false;
	return Actor != GetOwner();
}

// ReSharper disable once CppUE4CodingStandardNamingViolationWarning
bool UTargetingComponent::IsActorTargetPVPValid(AActor* TargetActor, AActor* SourceActor) const
{
	if (const AOnsetPlayerCharacter* TargetCharacter = Cast<AOnsetPlayerCharacter>(TargetActor); !TargetCharacter) return false;
	if (!SourceActor) return false;
	if (const AOnsetPlayerController* SourceController = Cast<AOnsetPlayerController>(SourceActor->GetInstigatorController()))
	{
		AOnsetPlayerState* SourcePlayerState = SourceController->GetPlayerState<AOnsetPlayerState>();
		if (SourcePlayerState && !SourcePlayerState->bIsPvPEnabled)	return false;
	}
	return true;
}

