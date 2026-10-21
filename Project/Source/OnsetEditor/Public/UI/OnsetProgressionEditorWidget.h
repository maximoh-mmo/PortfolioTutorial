// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "Data/OnsetEquipmentTypes.h"
#include "OnsetPlayerDataTypes.h"
#include "Progression/OnsetProgressionSimulator.h"
#include "Progression/OnsetLootPacingCalculator.h"
#include "OnsetProgressionEditorWidget.generated.h"

class UDataTable;
class UVerticalBox;
class UHorizontalBox;
class UScrollBox;
class UTextBlock;
class UButton;
class UOnsetNotifyDetailsView;

/**
 * Transient wrapper for Leveling & Progression curve settings.
 * Exposed via DetailsView for live editing.
 */
UCLASS(Transient)
class UOnsetProgressionCurveWrapper : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leveling Curve", meta = (ClampMin = "1.0", ClampMax = "1000.0"))
	float XPBase = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leveling Curve", meta = (ClampMin = "0.01", ClampMax = "0.50"))
	float XPGrowth = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leveling Curve", meta = (ClampMin = "10", ClampMax = "1000"))
	int32 KillsPerLevel = 180;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leveling Curve", meta = (ClampMin = "1", ClampMax = "10"))
	int32 StatPointsPerLevel = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mitigation Constants", meta = (ClampMin = "10.0", ClampMax = "500.0"))
	float KDefense = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mitigation Constants", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float KZoneTierScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mitigation Constants", meta = (ClampMin = "1.0", ClampMax = "50.0"))
	float HealthPerVitality = 10.0f;
};

/**
 * Transient wrapper for Combat Simulator ("The Holodeck") configuration.
 * Exposed via DetailsView for live editing.
 */
UCLASS(Transient)
class UOnsetCombatHolodeckWrapper : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Setup")
	EOnsetCharacterClass PlayerClass = EOnsetCharacterClass::DPS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Setup", meta = (ClampMin = "1", ClampMax = "200", UIMin = "1", UIMax = "200"))
	int32 PlayerLevel = 15;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Setup", meta = (ClampMin = "0", ClampMax = "5", UIMin = "0", UIMax = "5"))
	int32 GearTier = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Setup")
	EOnsetWeaponArchetype WeaponArchetype = EOnsetWeaponArchetype::Sword;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Setup")
	FName SelectedEnemyRow = TEXT("OrcBrute");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Setup", meta = (ClampMin = "0.5", ClampMax = "10.0"))
	float EnemyRankMultiplier = 1.0f; // 1.0 = Normal, 1.5 = Elite, 3.0 = Boss
};

/**
 * Transient wrapper for Loot & Drop Pacing configuration.
 * Exposed via DetailsView for live editing.
 */
UCLASS(Transient)
class UOnsetLootPacingWrapper : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tier Budget", meta = (ClampMin = "0", ClampMax = "5"))
	int32 Tier = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tier Budget", meta = (ClampMin = "1", ClampMax = "200"))
	int32 MinLevel = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tier Budget", meta = (ClampMin = "1", ClampMax = "200"))
	int32 MaxLevel = 19;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tier Budget", meta = (ClampMin = "1", ClampMax = "1000"))
	int32 TargetTierDrops = 55;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Table Testing")
	FName LootTableRowName = TEXT("ST_Tier1_Equip");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monte Carlo Simulation", meta = (ClampMin = "100", ClampMax = "100000"))
	int32 Iterations = 10000;
};

/**
 * Editor Utility Widget for Progression & Balance Workbench.
 * 4-Pane layout uniting Leveling curves, Combat Holodeck TTK simulation,
 * Loot Drop Pacing Monte Carlo validator, and two-way config/DataTable persistence.
 */
UCLASS()
class UOnsetProgressionEditorWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Progression Workbench")
	void OpenEditor();

	UFUNCTION(BlueprintCallable, Category = "Progression Workbench")
	void RecalculateSimulation();

	UFUNCTION(BlueprintCallable, Category = "Progression Workbench")
	void RunMonteCarloTest();

	UFUNCTION(BlueprintCallable, Category = "Progression Workbench")
	void PreviewChanges();

	UFUNCTION(BlueprintCallable, Category = "Progression Workbench")
	void RevertChanges();

	UFUNCTION(BlueprintCallable, Category = "Progression Workbench")
	void CommitChanges();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BuildUI();
	void LoadCurrentConfig();
	void HandleCurvePropertyChanged();
	void HandleHolodeckPropertyChanged();
	void HandleLootPropertyChanged();
	void UpdateLootTierDefaults(int32 NewTier);
	void PersistOnClose();

	// Wrappers
	UPROPERTY(Transient)
	TObjectPtr<UOnsetProgressionCurveWrapper> CurveWrapper;

	UPROPERTY(Transient)
	TObjectPtr<UOnsetCombatHolodeckWrapper> HolodeckWrapper;

	UPROPERTY(Transient)
	TObjectPtr<UOnsetLootPacingWrapper> LootPacingWrapper;

	// UI Details Views
	UPROPERTY(Transient)
	TObjectPtr<UOnsetNotifyDetailsView> CurvePropertyView;

	UPROPERTY(Transient)
	TObjectPtr<UOnsetNotifyDetailsView> HolodeckPropertyView;

	UPROPERTY(Transient)
	TObjectPtr<UOnsetNotifyDetailsView> LootPropertyView;

	// Pane 1 Text widgets
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TotalXpText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CumulativeKillsText;

	// Pane 2 Text widgets
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PlayerDamageHitText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EnemyMitigationText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NetHitDamageText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> AttacksPerSecondText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PlayerDpsText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TimeToKillText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HitsToKillText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EnemyDpsText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PlayerSurvivalText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HitsToKillPlayerText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PacingRatingText;

	// Pane 3 Text widgets
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LevelSpanText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> KillBudgetText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PlaytimeText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RequiredDropRateText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MonteCarloDropsText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MonteCarloFrequencyText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MonteCarloDroughtText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MonteCarloStatusText;

	// Status bar text
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusBarText;

	// Original baseline values for dirty check & diff preview
	float OriginalXPBase = 50.0f;
	float OriginalXPGrowth = 0.06f;
	int32 OriginalKillsPerLevel = 180;
	int32 OriginalStatPointsPerLevel = 3;
	float OriginalKDefense = 100.0f;
	float OriginalKZoneTierScale = 1.0f;
	float OriginalHealthPerVitality = 10.0f;

	int32 CachedLootTier = 1;
	bool bIsDirty = false;
};
