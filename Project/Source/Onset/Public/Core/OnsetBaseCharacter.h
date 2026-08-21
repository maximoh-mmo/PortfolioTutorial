// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Data/OnsetEquipmentTypes.h"
#include "OnsetPlayerDataTypes.h"
#include "OnsetBaseCharacter.generated.h"

class UOnsetMovementAttributeSet;
class UOnsetCombatAttributeSet;
class UOnsetCCDiminishingComponent;
class UOnsetInventoryComponent;
class UOnsetQuestComponent;
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
	/** Binds the ASC's actor info to this pawn (owner + avatar); safe to re-run on re-possess. */
	void InitAbilityActorInfo();
	virtual void PossessedBy(AController* NewController) override;
	virtual void Tick(float DeltaSeconds) override;
	
	/** Home location used as a reference for AI leash and roam behavior. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FTransform HomeTransform;
	
	/** Ability System Component for GAS integration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	/** Grants the class loadout once per pawn life (data-driven rows via UOnsetAbilityLibrary). */
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

	/** CC diminishing-returns tracker (Stun/Freeze duration reduction). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UOnsetCCDiminishingComponent> CCDiminishing;

	/** Shared inventory: equipped loadout + bag. Owner-only replication on player pawns. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UOnsetInventoryComponent> InventoryComponent;

	/** Server-authoritative quest tracker (player pawns). Inert on enemies/corpses. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
	TObjectPtr<UOnsetQuestComponent> QuestComponent;

	// --- Equipment / derived stats ---

	/** Applies the saved loadout (or falls back to the class default weapon) and recomputes stats. */
	void ApplyCharacterBuild(EOnsetCharacterClass Class, const FString& EquipmentJSON);

	/** Recomputes combat attributes + MaxHealth from class base stats and equipped gear. */
	void RecalculateDerivedStats();

	/** Serializes the current loadout as EquipmentJSON. */
	FString SerializeEquipmentJSON() const;

	/** Parses an EquipmentJSON string into the loadout. */
	void DeserializeEquipmentJSON(const FString& JSON);

	/** Serializes the bag contents as InventoryJSON. */
	FString SerializeInventoryJSON() const;

	/** Parses an InventoryJSON string into the bag contents. */
	void DeserializeInventoryJSON(const FString& JSON);

	/** Equips the DT_Equipment row RowName into Slot (server); empty removes it. */
	void EquipItem(EOnsetEquipmentSlot Slot, FName RowName);

	/** Moves RowName from the bag into its slot, replacing whatever is equipped there. */
	bool EquipFromInventory(FName RowName);

	/** The WeaponBase used by weapon-scaled abilities: equipped weapon or class-default fallback. */
	float GetEquippedWeaponDamage() const;

	/** The equipped item definition for Slot, or null when nothing is equipped. */
	const FOnsetEquipmentDefinition* GetEquippedItem(EOnsetEquipmentSlot Slot) const;

	EOnsetCharacterClass GetCharacterClass() const { return CurrentClass; }

	void SetCharacterClass(EOnsetCharacterClass InClass) { CurrentClass = InClass; }

	/**
	 * Sets the enemy-authored weapon values (DT_EnemyStats). Enemies have no equipment
	 * loadout, so GetEquippedWeaponDamage() returns EnemyWeaponBaseOverride and the
	 * basic-attack cooldown uses EnemyWeaponArchetype.
	 */
	void SetEnemyWeaponStats(float InWeaponBase, EOnsetWeaponArchetype InArchetype);

	/**
	 * The weapon archetype driving the basic-attack cooldown: equipped weapon when
	 * present, else the enemy's authored archetype, else the class default.
	 */
	EOnsetWeaponArchetype GetBaseWeaponArchetype() const;
		
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
	/** Guard so GrantDefaultAbilities only runs once per pawn (re-possess must not re-grant). */
	bool bAbilitiesGranted = false;

	/** Character class used to resolve base stats + the default weapon fallback (players). */
	UPROPERTY()
	EOnsetCharacterClass CurrentClass = EOnsetCharacterClass::DPS;

	/** Enemy-authored basic-attack WeaponBase (DT_EnemyStats); 0 = not an enemy override. */
	UPROPERTY()
	float EnemyWeaponBaseOverride = 0.0f;

	/** Enemy-authored basic-attack archetype (DT_EnemyStats); drives the cooldown table. */
	UPROPERTY()
	EOnsetWeaponArchetype EnemyWeaponArchetype = EOnsetWeaponArchetype::Sword;
	
	/** Whether the reticle decal is currently visible; drives the per-frame ground trace. */
	UPROPERTY(Transient)
	bool bTargetReticleVisible = false;

	/** Line-trace straight down and place the reticle decal on the first surface, following terrain. */
	void UpdateTargetReticule();

	UPROPERTY(ReplicatedUsing = OnRep_bIsAlive)
	bool bIsAlive = true;

private:
	/** Recomputes derived stats whenever the inventory component mutates. */
	void HandleInventoryChanged();
};
