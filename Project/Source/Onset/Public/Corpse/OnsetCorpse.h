// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OnsetCorpse.generated.h"

/** A corpse actor with a static mesh, spawned and recycled by UOnsetCorpseSubsystem. */
UCLASS()
class ONSET_API AOnsetCorpse : public AActor
{
	GENERATED_BODY()

public:
	AOnsetCorpse();

	/** Static mesh component for the corpse visual. */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> MeshComponent;
};
	