// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/OnsetPlayerState.h"

#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"
#include "Player/OnsetPlayerController.h"
#include "Core/TargetingComponent.h"
#include "GameFramework/Pawn.h"

void AOnsetPlayerState::OnRep_PvPEnabled()
{
	if (bIsPvPEnabled) return;
	AOnsetPlayerController* Controller = Cast<AOnsetPlayerController>(GetPlayerController());
	if (Controller)
	{
		APawn* Pawn = Controller->GetPawn();
		if (Pawn)
		{
			UTargetingComponent* TargetingComponent = Cast<UTargetingComponent>(
				Pawn->GetComponentByClass(UTargetingComponent::StaticClass()));
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
}

void AOnsetPlayerState::OnRep_AutoplayEnabled()
{
	UE_LOG(LogTemp, Warning, TEXT("OnRep_AutoplayEnabled: bAutoplayEnabled=%d (Net=%s, IsLocallyControlled=%d)"),
		bAutoplayEnabled ? 1 : 0,
		GetWorld() && GetWorld()->GetNetDriver() ? (GetWorld()->GetNetDriver()->IsServer() ? TEXT("Server") : TEXT("Client")) : TEXT("None"),
		GetPlayerController() ? GetPlayerController()->IsLocalController() ? 1 : 0 : 0);
	OnPlayerSettingsChanged.Broadcast();
}

void AOnsetPlayerState::OnRep_ContinueOnDisconnect()
{
	OnPlayerSettingsChanged.Broadcast();
}

void AOnsetPlayerState::OnRep_IdleAutoCombatDelay()
{
	OnPlayerSettingsChanged.Broadcast();
}

void AOnsetPlayerState::SetIdleAutoCombatDelaySeconds(float Seconds)
{
	const float Clamped = FMath::Max(0.0f, Seconds);
	if (!FMath::IsNearlyEqual(Clamped, IdleAutoCombatDelaySeconds))
	{
		IdleAutoCombatDelaySeconds = Clamped;
		OnPlayerSettingsChanged.Broadcast();
	}
}

void AOnsetPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AOnsetPlayerState, bIsPvPEnabled);
	DOREPLIFETIME(AOnsetPlayerState, bAutoplayEnabled);
	DOREPLIFETIME_CONDITION(AOnsetPlayerState, IdleAutoCombatDelaySeconds, COND_OwnerOnly);
	DOREPLIFETIME(AOnsetPlayerState, bContinueOnDisconnect);
	DOREPLIFETIME(AOnsetPlayerState, PlayerPlatform);
	DOREPLIFETIME(AOnsetPlayerState, PlayerPlatformID);
}

