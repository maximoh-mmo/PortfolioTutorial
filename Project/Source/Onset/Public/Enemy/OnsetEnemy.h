// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/OnsetBaseCharacter.h"
#include "GameplayTagContainer.h"
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
	
	/** Spawner that owns this slot; set at spawn, cleared on pool return (respawn routing). */
	UPROPERTY()
	TObjectPtr<AOnsetSpawner> OwningSpawner;

	/** Area tag from the owning spawner; gates zone-scoped loot entries. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	FGameplayTag ZoneTag;

	virtual void OnDeath(AActor* KillingActor = nullptr) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Client-side: re-applies the replicated VisualProfile's mesh/anim/material locally. */
	UFUNCTION()
	void OnRep_VisualProfile();

protected:

	/** Delayed death work (corpse spawn, pool return) so the death frame stays cheap. */
	void DeferredDeathCleanup();

	/** Difficulty tier set by ApplyEnemyStats; doubles as the loot level. */
	UPROPERTY()
	int32 DifficultyTier = 0;

	/** Authored enemy Level (DT_EnemyStats, 1-200); drives the XP LevelDiff multiplier. */
	UPROPERTY()
	int32 EnemyLevel = 1;

	/** Authored XP override (DT_EnemyStats); 0 = derive base XP from the curve. */
	UPROPERTY()
	int32 XpReward = 0;

	/** DT_EnemyStats row applied at spawn (for death-time loot lookup). */
	UPROPERTY()
	FName EnemyStatsRow;
};
