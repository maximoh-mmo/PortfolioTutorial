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
	void ProcessPrimaryInteraction(FVector2D ScreenPosition);
		
private:
	
	UPROPERTY()
	TObjectPtr<UTargetingComponent> TargetingComponent;
	
	
};
