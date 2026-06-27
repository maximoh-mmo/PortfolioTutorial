// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "OnsetBaseCharacter.generated.h"

class UOnsetMovementAttributeSet;
class UTargetingComponent;
class UOnsetAttributeSet;
class UAbilitySystemComponent;
/** Shared base for player and NPC characters. Used as a common type for targeting and ability systems. */
UCLASS(Blueprintable)
class ONSET_API AOnsetBaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AOnsetBaseCharacter();
	void InitAbilityActorInfo();
	virtual void PossessedBy(AController* NewController) override;
	
	/** Home location used as a reference for AI leash and roam behavior. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FTransform HomeTransform;
	
	/** Ability System Component for GAS integration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	void GrantDefaultAbilities();
	
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ResetAttributes();

	// --- Targeting ---

	/** Current target actor. Set by OnPerceptionUpdated for NPCs, by input for Player AI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	TObjectPtr<UTargetingComponent> TargetingComponent;
	
	/** Attribute set containing health, damage, and death event data. */
	UPROPERTY()
	TObjectPtr<UOnsetAttributeSet> AttributeSet;

	/** Attribute set containing movement speed and related properties. */
	UPROPERTY()
	TObjectPtr<UOnsetMovementAttributeSet> MovementAttributes;
		
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void OnDeath(AActor* KillingActor = nullptr);
	
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool IsAlive() const;
		
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void OnRespawn();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_bIsAlive();
	
protected:
	bool bAbilitiesGranted = false;
	
	UPROPERTY(ReplicatedUsing = OnRep_bIsAlive)
	bool bIsAlive = true;
};
