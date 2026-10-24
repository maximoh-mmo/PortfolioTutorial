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
class UOnsetNotifyDetailsView;
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
 * Transient UObject backing the ability-creation dialog.
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

	// --- Scroll ---

	/** Also create a DT_Scrolls row that grants this ability. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	bool bCreateScroll = false;

	/** Threat multiplier on this ability's damage (1.0 = normal; >1 = high-threat/taunt). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Threat")
	float ThreatMultiplier = 1.0f;

	// --- Effects ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	bool bDamage = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (EditCondition = "bDamage", EditConditionHides))
	float DamageAmount = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (EditCondition = "bDamage", EditConditionHides))
	EOnsetDamageElement DamageElement = EOnsetDamageElement::Physical;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (EditCondition = "bDamage", EditConditionHides))
	EOnsetScalingType DamageScaling = EOnsetScalingType::Weapon;

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
	EOnsetDamageElement DoTElement = EOnsetDamageElement::Physical;

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
	bool bFreeze = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (EditCondition = "bFreeze", EditConditionHides))
	float FreezeDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	bool bInvulnerable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (EditCondition = "bInvulnerable", EditConditionHides))
	float InvulnerableDuration = 2.0f;
};

/**
 * Editor Utility Widget that edits DT_Abilities.
 * Left: list of ability rows.
 * Right: PropertyView bound to selected row.
 * Bottom: Live DPS / HPS & Animation Sync metrics, Viewport Debug Range Visualizer, and Save button.
 */
UCLASS()
class UOnsetAbilityEditorWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Ability Editor")
	void OpenEditor();

	UFUNCTION(BlueprintCallable, Category = "Ability Editor")
	void RefreshFromTable();

	UFUNCTION(BlueprintCallable, Category = "Ability Editor")
	void AddDefinition();

	UFUNCTION(BlueprintCallable, Category = "Ability Editor")
	void DeleteDefinition();

	UFUNCTION(BlueprintCallable, Category = "Ability Editor")
	void SaveDefinition();

	/** Draws debug shapes for targeting range/radius and plays montage on preview actor if available. */
	UFUNCTION(BlueprintCallable, Category = "Ability Editor")
	void PreviewCastInViewport();

	void SelectRow(FName RowName);

protected:
	virtual void NativeConstruct() override;

private:
	void BuildUI();
	void RebuildList();
	bool WriteBackRow();
	void HandlePropertyEdited();
	void UpdateDpsMetrics();
	UDataTable* GetAbilityTable();

	UPROPERTY(Transient)
	TObjectPtr<UOnsetAbilityEditRowWrapper> EditWrapper;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityCreationData> PendingCreationData;

	FName SelectedRowName;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> CachedTable;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> RowListBox;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UOnsetAbilityRowButton>> RowButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USizeBox>> HeaderCells;

	TArray<float> ColumnWidths;

	UPROPERTY(Transient)
	TObjectPtr<UOnsetNotifyDetailsView> PropertyView;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DpsMetricsText;
};
