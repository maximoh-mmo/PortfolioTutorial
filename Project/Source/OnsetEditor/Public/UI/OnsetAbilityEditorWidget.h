// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimMontage.h"
#include "Components/Button.h"
#include "EditorUtilityWidget.h"
#include "Data/OnsetAbilityTypes.h"
#include "OnsetAbilityEditorWidget.generated.h"

class UDataTable;
class UVerticalBox;
class UDetailsView;
class UTextBlock;
class USizeBox;
class UOnsetAbilityRowButton;

/**
 * Transient UObject exposing a single row to the PropertyView so the details panel
 * auto-generates the edit form (asset pickers, enums, number fields).
 */
UCLASS(Transient)
class UOnsetAbilityEditRowWrapper : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	FOnsetAbilityDefinition Definition;
};

/**
 * Transient UObject backing the ability-creation dialog. The auto-generated details
 * form shows the identity fields, the type-gated range fields, and the effect
 * checkboxes (each with its conditional magnitude/duration fields).
 */
UCLASS(Transient)
class UAbilityCreationData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	TSoftObjectPtr<UTexture2D> AbilityIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	EOnsetAbilityType AbilityType = EOnsetAbilityType::SingleTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability", meta = (EditCondition = "AbilityType == EOnsetAbilityType::SingleTarget", EditConditionHides))
	float AttackRange = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability", meta = (EditCondition = "AbilityType == EOnsetAbilityType::AoE", EditConditionHides))
	float CastRange = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability", meta = (EditCondition = "AbilityType == EOnsetAbilityType::AoE || AbilityType == EOnsetAbilityType::PointBlankAoE || AbilityType == EOnsetAbilityType::Cone", EditConditionHides))
	float Radius = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability", meta = (EditCondition = "AbilityType == EOnsetAbilityType::Cone", EditConditionHides))
	float ConeHalfAngle = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	TSoftObjectPtr<UAnimMontage> Montage;

	/** Delay (seconds) after the montage starts before effects resolve. Only used when a Montage is set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float DamageTime = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float CooldownSeconds = 1.0f;

	// --- Effects ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	bool bDamage = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (EditCondition = "bDamage", EditConditionHides))
	float DamageAmount = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	bool bHeal = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (EditCondition = "bHeal", EditConditionHides))
	float HealAmount = 25.0f;

	// --- Damage over Time ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	bool bDamageOverTime = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (EditCondition = "bDamageOverTime", EditConditionHides))
	float DoTDamageAmount = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (EditCondition = "bDamageOverTime", EditConditionHides))
	float DoTDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (EditCondition = "bDamageOverTime", EditConditionHides))
	float DoTPeriod = 1.0f;

	// --- Heal over Time ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	bool bHealOverTime = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (EditCondition = "bHealOverTime", EditConditionHides))
	float HoTHealAmount = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (EditCondition = "bHealOverTime", EditConditionHides))
	float HoTDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (EditCondition = "bHealOverTime", EditConditionHides))
	float HoTPeriod = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	bool bSnare = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (EditCondition = "bSnare", EditConditionHides))
	float SnareMoveSpeedMult = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (EditCondition = "bSnare", EditConditionHides))
	float SnareDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	bool bSlow = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (EditCondition = "bSlow", EditConditionHides))
	float SlowCooldownMult = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (EditCondition = "bSlow", EditConditionHides))
	float SlowDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	bool bStun = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (EditCondition = "bStun", EditConditionHides))
	float StunDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	bool bInvulnerable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (EditCondition = "bInvulnerable", EditConditionHides))
	float InvulnerableDuration = 2.0f;
};

/**
 * Editor Utility Widget that edits DT_Abilities.
 *
 * Left: list of ability rows (Add/Delete buttons). Right: a PropertyView bound to the
 * selected row wrapped in a transient UObject - the details panel auto-generates the
 * form. Save persists the table and refreshes the UOnsetAbilityLibrary registry so PIE
 * picks up changes without recompiling.
 */
UCLASS()
class UOnsetAbilityEditorWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	/**
	 * Builds the UI (if needed) and loads the row list. Must be called BEFORE
	 * TakeWidget(): the UUserWidget rebuild path snapshots RootWidget during the
	 * first TakeWidget, so building here guarantees the tree exists first.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability Editor")
	void OpenEditor();

	/** Rebuilds the row list from the table. */
	UFUNCTION(BlueprintCallable, Category = "Ability Editor")
	void RefreshFromTable();

	/** Adds a new default row and selects it. */
	UFUNCTION(BlueprintCallable, Category = "Ability Editor")
	void AddDefinition();

	/** Deletes the currently selected row. */
	UFUNCTION(BlueprintCallable, Category = "Ability Editor")
	void DeleteDefinition();

	/** Saves the edited row back to the table, persists the asset, and refreshes the registry. */
	UFUNCTION(BlueprintCallable, Category = "Ability Editor")
	void SaveDefinition();

	/** Selects the given row and loads it into the PropertyView. */
	void SelectRow(FName RowName);

protected:
	virtual void NativeConstruct() override;

private:
	/** Builds the full UI (list + property view + buttons) if not already built. */
	void BuildUI();

	/** Rebuilds the row list. */
	void RebuildList();

	/** Reads edited values back from the wrapper into the cached table row. */
	bool WriteBackRow();

	/** Returns the ability DataTable, loading + caching if needed. */
	UDataTable* GetAbilityTable();

	/** Transient UObject shown in the PropertyView. */
	UPROPERTY(Transient)
	TObjectPtr<UOnsetAbilityEditRowWrapper> EditWrapper;

	/** Holds the creation dialog's form data while the modal window is open (GC-safe). */
	UPROPERTY(Transient)
	TObjectPtr<UAbilityCreationData> PendingCreationData;

	FName SelectedRowName;

	/** Cached table to edit rows without reloading. */
	UPROPERTY(Transient)
	TObjectPtr<UDataTable> CachedTable;

	/** The row-list container (left side). */
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> RowListBox;

	/** Row buttons by row name, for in-place selection-highlight updates. */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UOnsetAbilityRowButton>> RowButtons;

	/** The column-header cell boxes, so RebuildList can resize them with the columns. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<USizeBox>> HeaderCells;

	/** Current column widths (Name, Type, Input, Cooldown), recomputed per refresh. */
	TArray<float> ColumnWidths;

	/** The auto-generated property form (right side). */
	UPROPERTY(Transient)
	TObjectPtr<UDetailsView> PropertyView;
};
