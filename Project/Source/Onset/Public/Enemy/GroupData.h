#pragma once

#include "CoreMinimal.h"
#include "GroupData.generated.h"

/** Lightweight snapshot of a group's current state, computed on demand by UGroupManagerComponent. */
USTRUCT(BlueprintType)
struct FGroupData
{
	GENERATED_BODY()

	/** Average position of all alive members. */
	UPROPERTY()
	FVector Center = FVector::ZeroVector;

	/** Number of alive members in the group. */
	UPROPERTY()
	int32 AliveCount = 0;
};
