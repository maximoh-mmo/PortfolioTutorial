#pragma once
#include "StateTreeTaskBase.h"
#include "OnsetStateTreeContext.generated.h"

USTRUCT(BlueprintType)
struct FOnsetStateTreeContextData
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, Category = "Context")
	TObjectPtr<AActor> SelfActor = nullptr;
	
	UPROPERTY(VisibleAnywhere, Category = "Context")
	TObjectPtr<AActor> Target = nullptr;
	
	UPROPERTY(VisibleAnywhere, Category = "Context")
	float Health = 100.0f;
};

