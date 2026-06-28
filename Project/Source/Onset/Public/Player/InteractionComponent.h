// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"


struct FHitResult;
class UTargetingComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ONSET_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionComponent();
	
	/** Primary interaction: raycasts at screen position, branches on hit type. */
	void ProcessPrimaryInteraction(AActor* HitActor, FVector HitLocation);
	
	/** Returns the last processed move target from ProcessPrimaryInteraction. */
	FVector GetPendingMoveTarget() const { return PendingMoveTarget; }
		
private:
	
	UPROPERTY()
	TObjectPtr<UTargetingComponent> TargetingComponent;
	
	FVector PendingMoveTarget = FVector::ZeroVector;
	
	
};
