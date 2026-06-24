// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/OnsetBaseCharacter.h"
#include "Subsystems/WorldSubsystem.h"
#include "OnsetThreatSubsystem.generated.h"

class AOnsetEnemy;
class APlayerState;
/**
 * 
 */
UCLASS()
class ONSET_API UOnsetThreatSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// --- Threat ---
	
	// Add/Subtract threat.
	void AddThreat( AOnsetBaseCharacter* PlayerCharacter, AOnsetEnemy* Enemy, float ThreatAmount);
	// Clean up on player disconnect (after autoplay timeout/death if enabled).
	void RemovePlayer(const AOnsetBaseCharacter* PlayerCharacter);
	// Clean up Enemy on death.
	void RemoveEnemy(AOnsetEnemy* Enemy);
	// Return the highest threat player.
	APawn* GetPrimaryTarget(AOnsetEnemy* Enemy);
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
	
private:
    TMap<TWeakObjectPtr<AOnsetEnemy>, TMap<TWeakObjectPtr<AOnsetBaseCharacter>, float>> ThreatTable;
	TMap<TWeakObjectPtr<AOnsetBaseCharacter>, TArray<TWeakObjectPtr<AOnsetEnemy>>> EngagementTable;
};
