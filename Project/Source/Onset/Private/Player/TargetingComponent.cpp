// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/TargetingComponent.h"

#include "GameFramework/Actor.h"

UTargetingComponent::UTargetingComponent() : CurrentTarget(nullptr)
{
}

void UTargetingComponent::SetTarget(AActor* NewTarget)
{
	if (!IsActorTargetValid(NewTarget)) return;
	CurrentTarget = NewTarget;
	UE_LOG(LogTemp, Warning, TEXT("Setting target to %s"), *NewTarget->GetName());
}

void UTargetingComponent::ClearTarget()
{
	CurrentTarget = nullptr;
}

bool UTargetingComponent::IsActorTargetValid(AActor* Actor)
{
	return Actor != nullptr && Actor != GetOwner();
}

