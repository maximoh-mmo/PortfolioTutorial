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

AActor* UTargetingComponent::GetTarget() const
{
	if (CurrentTarget && !IsActorTargetValid(CurrentTarget))
	{
		return nullptr;
	}
	return CurrentTarget;
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

	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		Server_SetTarget(NewTarget);
	}
}

void UTargetingComponent::Server_SetTarget_Implementation(AActor* NewTarget)
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
	if (!CurrentTarget) return;

	CurrentTarget = nullptr;
	OnTargetChanged.Broadcast(CurrentTarget);

	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		Server_ClearTarget();
	}
}

void UTargetingComponent::Server_ClearTarget_Implementation()
{
	CurrentTarget = nullptr;
	OnTargetChanged.Broadcast(CurrentTarget);
}

bool UTargetingComponent::IsActorTargetValid(AActor* Actor) const
{
	if (!Actor || !IsValid(Actor)) return false;
	const AOnsetBaseCharacter* Character = Cast<AOnsetBaseCharacter>(Actor);
	if (!Character) return false;
	if (!Character->IsAlive()) return false;
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

