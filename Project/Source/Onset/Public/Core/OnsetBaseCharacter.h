// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "OnsetBaseCharacter.generated.h"

class UOnsetMovementAttributeSet;
class UOnsetCombatAttributeSet;
class UTargetingComponent;
class UOnsetAttributeSet;
class UAbilitySystemComponent;
class UDecalComponent;
class UMaterialInterface;
/** Broad category of a target, used by the target HUD to pick a lifebar skin. */
UENUM(BlueprintType)
enum class ETargetType : uint8
{
	Normal	UMETA(DisplayName = "Normal"),
	Elite	UMETA(DisplayName = "Elite"),
	Boss	UMETA(DisplayName = "Boss")
};

/** Shared base for player and NPC characters. Used as a common type for targeting and ability systems. */
UCLASS(Blueprintable)
class ONSET_API AOnsetBaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AOnsetBaseCharacter();
	void InitAbilityActorInfo();
	virtual void PossessedBy(AController* NewController) override;
	virtual void Tick(float DeltaSeconds) override;
	
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

	/** Attribute set containing combat stats (CooldownMultiplier for the Slow debuff). */
	UPROPERTY()
	TObjectPtr<UOnsetCombatAttributeSet> CombatAttributes;
		
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void OnDeath(AActor* KillingActor = nullptr);
	
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool IsAlive() const;
		
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void OnRespawn();

	// --- Target Reticle ---

	/** Ground reticle decal shown under this character while it is the player's target. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Target Reticle")
	TObjectPtr<UDecalComponent> TargetReticuleDecal;

	/**
	 * Material used by the ground reticle decal. Assignable per class (e.g. a boss or
	 * elite variant); falls back to the default ring material when left unset.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Reticle")
	TObjectPtr<UMaterialInterface> TargetReticuleMaterial;

	/** Broad category used by the target HUD to pick a lifebar skin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	ETargetType TargetType = ETargetType::Normal;

	/** Shows/hides the ground reticle decal, scaled to this character's capsule size. */
	UFUNCTION(BlueprintCallable, Category = "Target Reticle")
	void SetTargetReticule(bool bShow);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_bIsAlive();
	
protected:
	bool bAbilitiesGranted = false;
	
	/** Whether the reticle decal is currently visible; drives the per-frame ground trace. */
	UPROPERTY(Transient)
	bool bTargetReticleVisible = false;

	/** Line-trace straight down and place the reticle decal on the first surface, following terrain. */
	void UpdateTargetReticule();

	UPROPERTY(ReplicatedUsing = OnRep_bIsAlive)
	bool bIsAlive = true;
};
