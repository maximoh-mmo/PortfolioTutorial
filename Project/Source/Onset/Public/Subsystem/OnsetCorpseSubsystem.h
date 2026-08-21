// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OnsetCorpseSubsystem.generated.h"

class AOnsetCorpse;

/** Manages a pool of corpse actors, capping active count and cleaning up expired corpses. */
UCLASS(Config=Onset)
class ONSET_API UOnsetCorpseSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Spawn a corpse actor at the given transform with an optional static mesh. */
	AOnsetCorpse* SpawnCorpse(const FTransform& Transform, UStaticMesh* CorpseMesh = nullptr);

	/** Maximum number of active corpses before the oldest is swept. */
	UPROPERTY(Config)
	int32 MaxActiveCorpses = 20;

	/** Time in seconds before a corpse is automatically removed. */
	UPROPERTY(Config)
	float CorpseLifespan = 15.0f;

private:
	/** Evicts the oldest corpses while the active count exceeds MaxActiveCorpses. */
	void SweepDeadCorpses();

	/** Live corpse actors in spawn order (oldest first, for cap eviction). */
	TArray<TWeakObjectPtr<AOnsetCorpse>> ActiveCorpses;
};
