#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AbilityTargetingLibrary.generated.h"

class UTargetingComponent;

USTRUCT(BlueprintType)
struct FAbilityTargetData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Ability Targeting")
	AActor* TargetActor = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Ability Targeting")
	FVector TargetLocation = FVector::ZeroVector;

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
