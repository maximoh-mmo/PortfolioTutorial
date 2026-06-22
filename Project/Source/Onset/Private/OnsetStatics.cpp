// Fill out your copyright notice in the Description page of Project Settings.


#include "OnsetStatics.h"

#include "Kismet/GameplayStatics.h"
#include "Player/OnsetPlayerCharacter.h"
#include "Player/OnsetPlayerController.h"
#include "Player/OnsetPlayerState.h"
#include "Core/TargetingComponent.h"

DEFINE_LOG_CATEGORY(OnsetCore);

AOnsetPlayerController* UOnsetStatics::GetOnsetPlayerController(UObject* WorldContext, int32 PlayerIndex)
{
	if (const auto CastPlayer = Cast<AOnsetPlayerController>(UGameplayStatics::GetPlayerController(WorldContext, PlayerIndex))) return CastPlayer;
	UE_LOG(OnsetCore, Error, TEXT("Cast to AOnsetPlayerController Failed"));
	return nullptr;
}

AOnsetPlayerState* UOnsetStatics::GetOnsetPlayerState(UObject* WorldContext, int32 PlayerIndex)
{
	const auto Controller = GetOnsetPlayerController(WorldContext, PlayerIndex);
	if (!Controller) return nullptr;
	const auto PlayerState = Controller->PlayerState;
	if (!PlayerState) return nullptr;
	if (const auto CastState = Cast<AOnsetPlayerState>(PlayerState)) return CastState;
	UE_LOG(OnsetCore, Error, TEXT("Failed to cast PlayerState to AOnsetPlayerState"));
	return nullptr;	
}

UTargetingComponent* UOnsetStatics::GetTargetingComponent(AActor* Actor)
{
	if (UActorComponent* Component = Actor->GetComponentByClass(UTargetingComponent::StaticClass())){
		if (UTargetingComponent* CastComponent = Cast<UTargetingComponent>(Component)) return CastComponent;
	}
	return nullptr;
}

AOnsetPlayerCharacter* UOnsetStatics::GetOnsetPlayerCharacter(UObject* WorldContext, int32 PlayerIndex)
{
	const auto Controller = UGameplayStatics::GetPlayerController(WorldContext, PlayerIndex);
	if (!Controller) {
		UE_LOG(OnsetCore, Error, TEXT("Failed to get PlayerCharacter for PlayerIndex %d"), PlayerIndex);
		return nullptr;
	}
	const auto Pawn = Controller->GetPawn();
	if (!Pawn)
	{
		UE_LOG(OnsetCore, Error, TEXT("Failed to get Pawn"));
		return nullptr;
	}
	if (const auto CastCharacter = Cast<AOnsetPlayerCharacter>(Pawn)) return CastCharacter;
	return nullptr;
}

bool UOnsetStatics::IsPvPEnabled(UObject* WorldContext, int32 PlayerIndex)
{
	const auto State = GetOnsetPlayerState(WorldContext,PlayerIndex);
	if (!State) return false;
	return State->bIsPvPEnabled;
}
