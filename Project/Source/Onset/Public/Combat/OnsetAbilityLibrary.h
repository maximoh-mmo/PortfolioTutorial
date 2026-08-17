// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "Data/OnsetAbilityTypes.h"
#include "OnsetAbilityLibrary.generated.h"

class UDataTable;
struct FOnsetAbilityDefinition;

/**
 * Startup registry for DT_Abilities.
 *
 * Loads the ability DataTable once (lazily, on first access), caches every row in
 * a TMap keyed by RowName, and eagerly resolves each row's AbilityClass soft pointer.
 * ValidateDefinitions() is called at boot to fail fast on missing classes / bad rows
 * instead of surfacing errors mid-combat. All lookups after the first are plain map
 * hits. In the packaged game the DataTable ships inside the .pak as static content;
 * nothing is created or edited at runtime.
 */
UCLASS()
class ONSET_API UOnsetAbilityLibrary : public UObject
{
	GENERATED_BODY()

public:
	/** Path to DT_Abilities. Overridable via Onset.Gameplay AbilityDataTable in DefaultEngine.ini. */
	static FString GetAbilityDataTablePath();

	/** Returns the loaded ability DataTable, or null if missing. Loads+caches on first call. */
	static UDataTable* GetAbilityTable();

	/** Returns the cached definition for RowName, or null if the row is missing. */
	static const FOnsetAbilityDefinition* GetDefinition(FName RowName);

	/** Returns the AbilityID.<RowName> tag carried in a spec's DynamicAbilityTags. */
	static FGameplayTag MakeAbilityIDTag(FName RowName);

	/** Returns the Damage.<Element> tag for the given element enum, or Physical if invalid. */
	static FGameplayTag GetElementDamageTag(EOnsetDamageElement Element);

	/** Inverse of GetElementDamageTag: element enum for a Damage.* tag (Physical fallback). */
	static EOnsetDamageElement GetElementFromDamageTag(FGameplayTag DamageTag);

	/**
	 * Type-chart multiplier (1.5 / 1.0 / 0.5 / 0.0) for Source hitting a Target of the
	 * given affinity. Fire beats Ice, Ice beats Poison, Poison beats Lightning, Lightning
	 * beats Fire; same-element is 0.5; Physical is always 1.0. Structural default for the
	 * DT_ElementAffinity table (1.0 = Neutral fallback).
	 */
	static float GetElementAffinityMultiplier(EOnsetDamageElement Source, EOnsetDamageElement Target);

	/**
	 * Scans DynamicTags for the AbilityID.* tag and returns the matching row definition.
	 * Used by UOnsetGA_Generic to resolve its own row from the granted spec.
	 */
	static const FOnsetAbilityDefinition* GetDefinitionFromDynamicTags(const FGameplayTagContainer& DynamicTags);

	/**
	 * Resolves and loads the AbilityClass soft pointer for RowName. Returns null
	 * (and logs an error) if the row or its class is missing/unloadable.
	 */
	static TSubclassOf<class UOnsetGameplayAbility> ResolveAbilityClass(FName RowName);

	/** Iterates all rows, validates classes + effect types, logs failures loudly. Returns true if all rows are valid. */
	static bool ValidateDefinitions();

	/** Clears the cached table + row map (used by the editor tool after saving). */
	static void Refresh();

private:
	static UDataTable* LoadTable();
};
