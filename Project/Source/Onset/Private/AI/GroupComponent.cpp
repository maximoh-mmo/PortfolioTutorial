// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/GroupComponent.h"

#include "AI/GroupManagerComponent.h"
#include "AI/OnsetEnemy.h"

UGroupComponent::UGroupComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGroupComponent::RegisterWithGroup(UGroupManagerComponent* Manager)
{
	GroupManager = Manager;
}

void UGroupComponent::ClearGroup()
{
	GroupManager = nullptr;
}

void UGroupComponent::UnregisterFromGroup()
{
	if (GroupManager.IsValid())
	{
		if (const auto Enemy = Cast<AOnsetEnemy>(GetOwner()))
		{
			GroupManager->UnregisterMember(Enemy);
		}
		GroupManager = nullptr;
	}
}

FGroupData UGroupComponent::GetGroupData() const
{
	if (GroupManager.IsValid())
	{
		return GroupManager->GetGroupData();
	}
	return FGroupData();
}
