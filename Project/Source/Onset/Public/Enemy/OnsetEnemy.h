// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/OnsetBaseCharacter.h"
#include "OnsetEnemy.generated.h"

class UGroupComponent;
class UVisualProfile;
class AOnsetSpawner;

/** NPC pawn owned by AOnsetAIController. Visuals are driven by UAIProfile via ApplyProfile(). */
UCLASS()
class ONSET_API AOnsetEnemy : public AOnsetBaseCharacter
{
	GENERATED_BODY()

public:
	AOnsetEnemy();

	/** Applies or clears the profile — sets mesh, anim BP, material, and capsule size. */
	UFUNCTION(BlueprintCallable, Category = "AI")
	void ApplyProfile(UVisualProfile* InProfile);

	/**
	 * Applies the DT_EnemyStats row (Phase 7): MaxHealth/DamageBase scaled by
	 * (1 + d)^Tier, DEF/RES/LUK, the basic-attack weapon base + archetype, and the
	 * Element.* affinity tag for the type chart. A missing row resets to defaults.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI")
	void ApplyEnemyStats(FName RowName, int32 Tier);
	
	/** Group membership component. Pawn-level bridge to UGroupManagerComponent. */
	UPROPERTY()
	TObjectPtr<UGroupComponent> GroupComp;
	
	/** Visual profile defining mesh, anim BP, material, and capsule size. Applied via ApplyProfile(). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Profile", ReplicatedUsing = OnRep_VisualProfile)
	TObjectPtr<UVisualProfile> VisualProfile;
	
	UPROPERTY()
	TObjectPtr<AOnsetSpawner> OwningSpawner;

	virtual void OnDeath(AActor* KillingActor = nullptr) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_VisualProfile();

protected:

	void DeferredDeathCleanup();
};
