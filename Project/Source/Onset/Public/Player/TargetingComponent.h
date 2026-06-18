// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TargetingComponent.generated.h"

/**
 * Pure data holder for the player's current target with accessor validation.
 */
UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class ONSET_API UTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UTargetingComponent();

	// --- Target Accessors ---

	/** Sets the current target. Pass nullptr to clear. */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	void SetTarget(AActor* NewTarget = nullptr);
	
	/** Returns the current target actor, or nullptr if no target is set. */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	AActor* GetTarget() const { return CurrentTarget; };
	
	/** Returns true if a valid target is currently set. */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	bool HasTarget() const { return CurrentTarget != nullptr; };
	
	/** Clears the current target. Identical to SetTarget(nullptr). */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	void ClearTarget();
	
	/** Target validation determines whether a target should be set */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	bool IsActorTargetValid(AActor* Actor);
	/** Target validation override for pvp checks */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	bool IsActorTargetPVPValid(AActor* TargetActor, AActor* SourceActor);

private:
	/** The currently targeted actor. Updated by PlayerController context resolution. */
	UPROPERTY(VisibleAnywhere, Category = "Targeting")
	AActor* CurrentTarget;	
};
