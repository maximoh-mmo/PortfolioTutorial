// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PerceptionProfile.generated.h"

/** Data asset that defines an NPC variant: Perception parameters. */
UCLASS()
class ONSET_API UPerceptionProfile : public UDataAsset
{
	GENERATED_BODY()
	
public:
	/** Maximum distance at which sight sense can detect targets. */
	UPROPERTY(EditAnywhere, Category = "Perception")
	float SightRange = 1500.0f;

	/** Horizontal field-of-view angle for sight detection (degrees). */
	UPROPERTY(EditAnywhere, Category = "Perception")
	float SightAngle = 90.0f;

	/** Maximum distance at which hearing sense can detect noise events. */
	UPROPERTY(EditAnywhere, Category = "Perception")
	float HearingRange = 800.0f;
};
