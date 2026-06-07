#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OnsetCorpseSubsystem.generated.h"

class AOnsetCorpse;

UCLASS(Config=Onset)
class ONSET_API UOnsetCorpseSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	AOnsetCorpse* SpawnCorpse(const FTransform& Transform, UStaticMesh* CorpseMesh = nullptr);

	UPROPERTY(Config)
	int32 MaxActiveCorpses = 20;

	UPROPERTY(Config)
	float CorpseLifespan = 15.0f;

private:
	void SweepDeadCorpses();

	TArray<TWeakObjectPtr<AOnsetCorpse>> ActiveCorpses;
};
