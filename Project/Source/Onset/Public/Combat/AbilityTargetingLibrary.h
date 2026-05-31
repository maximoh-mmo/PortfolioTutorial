// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AbilityTargetingLibrary.generated.h"

class UTargetingComponent;

/** Ability targeting info derived from the targeting component and source actor. */
USTRUCT(BlueprintType)
struct FAbilityTargetData
{
	GENERATED_BODY()

	/** The targeted actor, or null if targeting a location. */
	UPROPERTY(BlueprintReadWrite, Category = "Ability Targeting")
	AActor* TargetActor = nullptr;

	/** World location of the target (actor position or ground hit). */
	UPROPERTY(BlueprintReadWrite, Category = "Ability Targeting")
	FVector TargetLocation = FVector::ZeroVector;

	/** Normalized direction from source to target. */
	UPROPERTY(BlueprintReadWrite, Category = "Ability Targeting")
	FVector TargetDirection = FVector::ZeroVector;
};

UCLASS()
class ONSET_API UAbilityTargetingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Ability Targeting")
	static FAbilityTargetData GetTargetData(UTargetingComponent* TargetingComponent, AActor* SourceActor);
};
