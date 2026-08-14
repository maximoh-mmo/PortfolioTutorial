// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
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
