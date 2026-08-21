// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/OnsetBaseCharacter.h"
#include "Subsystems/WorldSubsystem.h"
#include "OnsetThreatSubsystem.generated.h"

class AOnsetEnemy;

/**
 * Server-side threat tables driving NPC target selection (see [Threat_System.md]).
 * Per-enemy threat map + per-player engagement lists; both keyed by weak pointers so
 * destroyed actors decay naturally. All mutations are authority-only (client calls no-op).
 */
UCLASS()
class ONSET_API UOnsetThreatSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// --- Threat ---

	/** Adds (or subtracts, via negative) threat for PlayerCharacter on Enemy; clamped at 0. */
	void AddThreat(AOnsetBaseCharacter* PlayerCharacter, AOnsetEnemy* Enemy, float ThreatAmount);
	// Clean up on player disconnect (after autoplay timeout/death if enabled).
	void RemovePlayer(const AOnsetBaseCharacter* PlayerCharacter);
	// Clean up Enemy on death.
	void RemoveEnemy(AOnsetEnemy* Enemy);
	// Return the highest threat player (by raw value).
	APawn* GetPrimaryTarget(AOnsetEnemy* Enemy);
	/** Highest threat × distance weight: 1.0 inside AttackRange, 0.5 inside ChaseRange,
	 *  0.1 beyond. Ties break toward the first-encountered entry. */
	AOnsetBaseCharacter* GetBestTarget(AOnsetEnemy* Enemy, float AttackRange, float ChaseRange);
	// Return the given ranked threat player.
	APawn* GetNthTarget(int32 Rank, AOnsetEnemy* Enemy);
	// Return the Given Player's current threat position (0 = highest, -1 = invalid).
	int32 GetTargetRank(AOnsetEnemy* Enemy, AOnsetBaseCharacter* PlayerCharacter);
	// Return the number of players on threat list
	int32 GetTargetCount(AOnsetEnemy* Enemy);
	// Level Transition.                
	void ClearAll();
	
	// --- Engagement ---
	
	// Adds Enemy to Player's engagement Array.
	void RegisterEngaged(AOnsetBaseCharacter* PlayerCharacter, AOnsetEnemy* Enemy);
	// Removes Enemy from all engagement Arrays.
	void UnregisterEngaged(AOnsetEnemy* Enemy);
	// Returns the number of enemies engaged with the given player character.
	int32 GetEngagedCount(AOnsetBaseCharacter* PlayerCharacter);
	// Returns the index of this Enemy as stored against the Player character.
	int32 GetEngagedIndex(AOnsetEnemy* Enemy, AOnsetBaseCharacter* PlayerCharacter);
	// Switch this Enemy's engagement from its current player(s) to NewPlayer.
	void SwitchTarget(AOnsetEnemy* Enemy, AOnsetBaseCharacter* NewPlayer);
	/** True when Enemy is currently registered in PlayerCharacter's engagement list. */
	bool IsEnemyEngagedWithPlayer(AOnsetEnemy* Enemy, AOnsetBaseCharacter* PlayerCharacter) const;

private:
	/** Enemy → (player → accumulated threat). Drives GetPrimaryTarget/GetBestTarget/GetNthTarget. */
	TMap<TWeakObjectPtr<AOnsetEnemy>, TMap<TWeakObjectPtr<AOnsetBaseCharacter>, float>> ThreatTable;
	/** Player → enemies currently engaged with them; index doubles as the angular-spread rank. */
	TMap<TWeakObjectPtr<AOnsetBaseCharacter>, TArray<TWeakObjectPtr<AOnsetEnemy>>> EngagementTable;
};