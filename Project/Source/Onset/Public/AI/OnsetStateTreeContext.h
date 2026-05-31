#pragma once
#include "StateTreeTaskBase.h"
#include "OnsetStateTreeContext.generated.h"

/** Per-NPC context data written every tick by FOnsetStateTreeContextTask. */
USTRUCT(BlueprintType)
struct FOnsetStateTreeContextData
{
	GENERATED_BODY()

	/** The pawn this StateTree is running on. */
	UPROPERTY(VisibleAnywhere, Category = "Context")
	TObjectPtr<AActor> SelfActor = nullptr;

	/** Current target actor, sourced from the controller's TargetingComponent. */
	UPROPERTY(VisibleAnywhere, Category = "Context")
	TObjectPtr<AActor> Target = nullptr;

	/** Current health value. Stub until GAS (A4) provides the attribute set. */
	UPROPERTY(VisibleAnywhere, Category = "Context")
	float Health = 100.0f;
};

