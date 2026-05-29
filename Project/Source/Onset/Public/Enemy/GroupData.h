#pragma once

#include "CoreMinimal.h"
#include "GroupData.generated.h"

USTRUCT(BlueprintType)
struct FGroupData
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Center = FVector::ZeroVector;

	UPROPERTY()
	int32 AliveCount = 0;

	UPROPERTY(EditDefaultsOnly)
	float AssistRadius = 800.0f;
};
