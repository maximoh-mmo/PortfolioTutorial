// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TargetingComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetChanged, AActor*, NewTarget);

/**
 * Target management component with validation and accessor methods.
 */
UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class ONSET_API UTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UTargetingComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- Target Accessors ---

	/** Sets the current target. Pass nullptr to clear. */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	void SetTarget(AActor* NewTarget = nullptr);
	
	/** Returns the current target actor, or nullptr if no target is set. */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	AActor* GetTarget() const;
	
	/** Returns true if a valid target is currently set. */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	bool HasTarget() const { return GetTarget() != nullptr; }
	
	/** Clears the current target. Identical to SetTarget(nullptr). */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	void ClearTarget();
	
	/** Target validation determines whether a target should be set */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	bool IsActorTargetValid(AActor* Actor) const;
	/** Target validation override for pvp checks */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	bool IsActorTargetPVPValid(AActor* TargetActor, AActor* SourceActor) const;

	/** Broadcasts whenever the target is set or cleared. */
	UPROPERTY(BlueprintAssignable, Category = "Targeting")
	FOnTargetChanged OnTargetChanged;

	UFUNCTION(Server, Reliable)
	void Server_SetTarget(AActor* NewTarget);

	UFUNCTION(Server, Reliable)
	void Server_ClearTarget();

private:
	/** Replays OnTargetChanged on clients when the replicated target updates. */
	UFUNCTION()
	void OnRep_CurrentTarget();

	/** The currently targeted actor. */
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_CurrentTarget, Category = "Targeting")
	TObjectPtr<AActor> CurrentTarget;	
};
