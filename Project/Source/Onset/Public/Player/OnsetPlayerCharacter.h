// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/OnsetBaseCharacter.h"
#include "OnsetPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProgressionChanged, int32, NewLevel, int32, NewExperience);

/**
 * The player-controlled character. Handles camera setup; input and targeting
 * are owned by the PlayerController. Owns the replicated level/XP progression
 * (combat-formulas §12) because the pawn outlives the PlayerState during
 * autoplay / continue-on-disconnect.
 */
UCLASS(Blueprintable)
class ONSET_API AOnsetPlayerCharacter : public AOnsetBaseCharacter
{
	GENERATED_BODY()

public:
	AOnsetPlayerCharacter();

	virtual void OnDeath(AActor* KillingActor = nullptr) override;
	
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void RespawnPlayer();
	
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float AttackRange = 250.0f;
	
	void EnableCameraLag(bool bEnable);

	// --- Progression (combat-formulas §12) ---

	/** Current level, replicated server->client. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Progression")
	int32 Level = 1;

	/** XP accumulated toward the next level, replicated server->client. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Progression")
	int32 Experience = 0;

	/** Unspent stat points awarded on level-up (spending UI is out of scope). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Progression")
	int32 UnspentStatPoints = 0;

	/** Broadcasts whenever Level or Experience changes (server -> UI binds here). */
	UPROPERTY(BlueprintAssignable, Category = "Progression")
	FOnProgressionChanged OnProgressionChanged;

	/**
	 * Grants XP for killing an enemy of EnemyLevel whose row authorizes XpReward
	 * (0 = derive from the curve). Applies the grey/yellow/green level-diff
	 * multiplier, handles level-ups (full heal + stat points), and persists.
	 * Server-only.
	 */
	UFUNCTION(BlueprintCallable, Category = "Progression")
	void GrantXPFromEnemy(int32 EnemyLevel, int32 XpReward);

	/** Applies the persisted progression values to this pawn (server, on select/possess). */
	void ApplyCharacterProgression(int32 InLevel, int32 InExperience, int32 InUnspentStatPoints);

	/**
	 * Grants flat XP (quest rewards). Runs the normal level-up pipeline and persists.
	 * Server-only.
	 */
	UFUNCTION(BlueprintCallable, Category = "Progression")
	void GrantQuestXP(int32 Amount);

	/**
	 * Stores the account identity this pawn persists under. Set on select/possess and on
	 * abandoned-pawn adoption, because once the AI controller owns the pawn (autoplay /
	 * continue-on-disconnect) GetPlayerState() returns null and PersistProgression must not
	 * rely on it. Server-only.
	 */
	void SetPersistIdentity(const FString& Platform, const FString& PlatformID, int32 SlotIndex);

	/** XP required to advance from the current level (curve, §12). */
	UFUNCTION(BlueprintCallable, Category = "Progression")
	int32 GetXPRequiredForNextLevel() const;

	/** 0..1 fraction of the current level's XP bar. */
	UFUNCTION(BlueprintCallable, Category = "Progression")
	float GetXPProgressPercent() const;

protected:
	// --- Camera ---
	
	/** Spring arm that provides the top-down view angle and collision push-back. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Camera attached to the spring arm socket. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

private:
	/** Adds XP and processes any level-ups; does NOT persist (GrantXPFromEnemy does). */
	void AddExperience(int32 Amount);

	/** Server-side: persists the current progression to the identity cache. */
	void PersistProgression();

	/** Account identity for persistence; survives AI possession (see SetPersistIdentity). */
	UPROPERTY()
	FString PersistPlatform;

	UPROPERTY()
	FString PersistPlatformID;

	UPROPERTY()
	int32 PersistSlotIndex = -1;
};
