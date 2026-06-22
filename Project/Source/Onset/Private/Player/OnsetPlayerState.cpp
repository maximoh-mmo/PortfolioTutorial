// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/OnsetPlayerState.h"

#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"
#include "Player/OnsetPlayerController.h"
#include "Core/TargetingComponent.h"

void AOnsetPlayerState::OnRep_PvPEnabled()
{
	if (bIsPvPEnabled) return;
	AOnsetPlayerController* Controller = Cast<AOnsetPlayerController>(GetPlayerController());
	if (Controller)
	{
		UTargetingComponent* TargetingComponent = Cast<UTargetingComponent>(Controller->GetComponentByClass(UTargetingComponent::StaticClass()));
		if (TargetingComponent)
		{
			AActor* Target = TargetingComponent->GetTarget();
			if (Target && Target->ActorHasTag("Player"))
			{
				TargetingComponent->ClearTarget();
			}
		}
	}
}

void AOnsetPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AOnsetPlayerState, bIsPvPEnabled);
}

