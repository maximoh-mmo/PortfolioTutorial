// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "EditorUtilityWidget.h"
#include "Data/OnsetEquipmentTypes.h"
#include "OnsetEnemyEditorWidget.generated.h"

class UDataTable;
class UVerticalBox;
class UDetailsView;
class UTextBlock;
class UOnsetAbilityRowButton;
class UOnsetNotifyDetailsView;
class AOnsetAppearancePreviewActor;

/**
 * Transient UObject exposing a single DT_EnemyStats row to the PropertyView so the
 * details panel auto-generates the whole form (stats, XP, loot table, modular appearance,
 * equipped combat abilities, and the three profile asset pickers).
 */
UCLASS(Transient)
class UOnsetEnemyEditRowWrapper : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	FOnsetEnemyStats Stats;
};

/** Transient form data for the enemy-creation dialog. */
UCLASS(Transient)
class UOnsetEnemyCreationData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	FText DisplayName;

	/** Creates sibling VP_/AI_/PP_<Name> profile assets linked into the new row. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	bool bAutoCreateProfiles = true;
};

/**
 * Editor Utility Widget that edits DT_EnemyStats.
 * Left: enemy list (Add/Delete).
 * Right: PropertyView bound to the selected row with immediate write-back.
 * Bottom: Multi-tier stat & TTK scaling matrix, Live In-Viewport Appearance Preview, and Test in PIE.
 */
UCLASS()
class UOnsetEnemyEditorWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Enemy Editor")
	void OpenEditor();

	UFUNCTION(BlueprintCallable, Category = "Enemy Editor")
	void RefreshFromTable();

	UFUNCTION(BlueprintCallable, Category = "Enemy Editor")
	void AddDefinition();

	UFUNCTION(BlueprintCallable, Category = "Enemy Editor")
	void DeleteDefinition();

	/** Spawns the selected enemy at the camera in a running PIE session. */
	UFUNCTION(BlueprintCallable, Category = "Enemy Editor")
	void TestInPIE();

	/** Spawns or updates a transient preview actor in the level editor viewport. */
	UFUNCTION(BlueprintCallable, Category = "Enemy Editor")
	void PreviewAppearanceInViewport();

	/** Removes any spawned preview actor from the level viewport. */
	UFUNCTION(BlueprintCallable, Category = "Enemy Editor")
	void CleanupPreviewActor();

	/** Selects the given row and loads it into the PropertyView. */
	void SelectRow(FName RowName);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BuildUI();
	void RebuildList();
	bool WriteBackRow();
	void UpdatePreviewText();
	UDataTable* GetEnemyStatsTable();
	void SaveTableAndProfiles(const TArray<UObject*>& ObjectsToSave);
	void HandlePropertyEdited();
	void PersistOnClose();

	UPROPERTY(Transient)
	TObjectPtr<UOnsetEnemyEditRowWrapper> EditWrapper;

	UPROPERTY(Transient)
	TObjectPtr<UOnsetEnemyCreationData> PendingCreationData;

	FName SelectedRowName;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> CachedTable;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> RowListBox;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UOnsetAbilityRowButton>> RowButtons;

	UPROPERTY(Transient)
	TObjectPtr<UOnsetNotifyDetailsView> PropertyView;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PreviewText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ScalingMatrixText;

	TWeakObjectPtr<AOnsetAppearancePreviewActor> CachedPreviewActor;

	/** Set when any edit / add / delete makes the table differ from disk. */
	bool bDirty = false;
};
