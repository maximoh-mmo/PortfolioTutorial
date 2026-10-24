// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/OnsetBaseCharacter.h"
#include "Data/OnsetRankTypes.h"
#include "OnsetPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UOnsetMovementValidationComponent;

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

	virtual void OnConstruction(const FTransform& Transform) override;

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

	/** Prestige level (increments when reaching LevelCap). Replicated server->client. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Progression")
	int32 PrestigeLevel = 0;

	/** Broadcasts whenever Level or Experience changes (server -> UI binds here). */
	UPROPERTY(BlueprintAssignable, Category = "Progression")
	FOnProgressionChanged OnProgressionChanged;

	/** Broadcasts when PrestigeLevel increments. */
	UPROPERTY(BlueprintAssignable, Category = "Progression")
	FOnProgressionChanged OnPrestigeChanged;

	// --- Confederate Rank & Protocol ---

	/** Current Confederate rank of the player (replicated server->client). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Rank")
	EOnsetRankId MilitaryRank = EOnsetRankId::CadetInductee;

	/** Service or governance branch. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Rank")
	EOnsetHierarchyBranch HierarchyBranch = EOnsetHierarchyBranch::Military;

	/** Functional specialism. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Rank")
	EOnsetSpecialism Specialism = EOnsetSpecialism::ShockAssault;

	/** Promotes or assigns a new Confederate rank to the player. Server-only. */
	UFUNCTION(BlueprintCallable, Category = "Rank")
	void SetMilitaryRank(EOnsetRankId NewRank);

	/** Plays protocol gesture animation/stance or visual feedback. */
	UFUNCTION(NetMulticast, Unreliable, Category = "Rank")
	void Multicast_PlayProtocolGesture(EOnsetProtocolStyle Style);

	/**
	 * Grants XP for killing an enemy of EnemyLevel whose row authorizes XpReward
	 * (0 = derive from the curve). Applies the grey/yellow/green level-diff
	 * multiplier, handles level-ups (full heal + stat points), and persists.
	 * Server-only.
	 */
	UFUNCTION(BlueprintCallable, Category = "Progression")
	void GrantXPFromEnemy(int32 EnemyLevel, int32 XpReward);

	/** Applies the persisted progression values to this pawn (server, on select/possess). */
	void ApplyCharacterProgression(int32 InLevel, int32 InExperience, int32 InUnspentStatPoints, int32 InPrestigeLevel);

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
    bool MatchesPersistIdentity(const FString& Platform, const FString& PlatformID, int32 SlotIndex) const { return PersistPlatform==Platform && PersistPlatformID==PlatformID && PersistSlotIndex==SlotIndex; }

	/** XP required to advance from the current level (curve, §12). */
	UFUNCTION(BlueprintCallable, Category = "Progression")
	int32 GetXPRequiredForNextLevel() const;

	/** 0..1 fraction of the current level's XP bar. */
	UFUNCTION(BlueprintCallable, Category = "Progression")
	float GetXPProgressPercent() const;

	/** Called when the prestige quest is completed. Resets level/XP, increments PrestigeLevel, applies multiplier. Server-only. */
	UFUNCTION(BlueprintCallable, Category = "Progression")
	void PrestigeUp();

	/** Server-side movement validation (delta budget, wall probes, cheat flagging). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement Validation")
	TObjectPtr<UOnsetMovementValidationComponent> MovementValidator;

protected:
	// --- Camera ---

	/** Spring arm that provides the top-down view angle and collision push-back. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Camera attached to the spring arm socket. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	/** Distance of the camera from the player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float CameraTargetArmLength = 1500.f;

	/** Pitch/Yaw/Roll rotation of the camera boom. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FRotator CameraRotation = FRotator(-46.f, 0.f, 0.f);

	/** Field of View for the camera (lower values create an isometric look). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float CameraFOV = 60.f;

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
