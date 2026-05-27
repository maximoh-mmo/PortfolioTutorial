// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/TargetingComponent.h"

UTargetingComponent::UTargetingComponent() : CurrentTarget(nullptr)
{
}

void UTargetingComponent::SetTarget(AActor* NewTarget)
{
	if (NewTarget == nullptr || NewTarget == CurrentTarget) return;
	CurrentTarget = NewTarget;
}

void UTargetingComponent::ClearTarget()
{
	CurrentTarget = nullptr;
}
