// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "EditorUtilityWidget.h"
#include "Data/OnsetEquipmentTypes.h"
#include "Data/OnsetItemTypes.h"
#include "Data/OnsetLootTypes.h"
#include "OnsetItemEditorWidget.generated.h"

class UDataTable;
class UVerticalBox;
class UDetailsView;
class UTextBlock;
class UHorizontalBox;
class UOnsetAbilityRowButton;
class UOnsetNotifyDetailsView;

/** Editor mode = one row struct + one DataTable. */
UENUM()
enum class EOnsetItemEditorMode : uint8
{
	Equipment,
	QuestItem,
	Junk,
	Scroll,
	LootTable
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnItemModeSelected, EOnsetItemEditorMode);

/** Mode-tab button; carries its mode and broadcasts selection. */
UCLASS()
class UOnsetItemModeButton : public UButton
{
	GENERATED_BODY()

public:
	EOnsetItemEditorMode Mode = EOnsetItemEditorMode::Equipment;
	FOnItemModeSelected OnModeSelected;

	UFUNCTION()
	void HandleClicked() { OnModeSelected.Broadcast(Mode); }
};

/** Transient wrapper exposing a single DT_Equipment row to the PropertyView. */
UCLASS(Transient)
class UOnsetEquipmentRowWrapper : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FOnsetEquipmentDefinition Row;
};

/** Transient wrapper exposing a single DT_QuestItems row to the PropertyView. */
UCLASS(Transient)
class UOnsetQuestItemRowWrapper : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest Item")
	FOnsetQuestItemDefinition Row;
};

/** Transient wrapper exposing a single DT_Junk row to the PropertyView. */
UCLASS(Transient)
class UOnsetJunkRowWrapper : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Junk")
	FOnsetJunkItemDefinition Row;
};

/** Transient wrapper exposing a single DT_Scrolls row to the PropertyView. */
UCLASS(Transient)
class UOnsetScrollRowWrapper : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scroll")
	FOnsetScrollDefinition Row;
};

/** Transient wrapper exposing a single DT_Loot row to the PropertyView. */
UCLASS(Transient)
class UOnsetLootRowWrapper : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	FOnsetLootTableRow Row;
};

/**
 * Editor Utility Widget that edits the four item category tables plus DT_Loot.
 * Top: mode tabs (Equipment / Quest Items / Junk / Scrolls / Loot Tables).
 * Left: row list. Right: PropertyView bound to the selected row wrapper. Edits
 * are written back immediately and persisted when the editor window closes (no
 * Save button). Every FDataTableRowHandle reference (item rows, granted
 * abilities, sub-tables) is validated before persisting. Loot mode adds a roll
 * preview (expected drop quantities over N rolls).
 */
UCLASS()
class UOnsetItemEditorWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Item Editor")
	void OpenEditor();

	UFUNCTION(BlueprintCallable, Category = "Item Editor")
	void RefreshFromTable();

	UFUNCTION(BlueprintCallable, Category = "Item Editor")
	void AddDefinition();

	UFUNCTION(BlueprintCallable, Category = "Item Editor")
	void DeleteDefinition();

	/** Switches the editor to the given mode and reloads the list. */
	UFUNCTION(BlueprintCallable, Category = "Item Editor")
	void SetMode(EOnsetItemEditorMode NewMode);

	/** Switches to the mode carried by the clicked tab button. */
	UFUNCTION()
	void OnModeButtonClicked(EOnsetItemEditorMode NewMode) { SetMode(NewMode); }

	/** Rolls the selected loot table N times and shows expected drop quantities. */
	UFUNCTION(BlueprintCallable, Category = "Item Editor")
	void RollPreview();

	void SelectRow(FName RowName);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BuildUI();
	void RebuildList();
	void UpdateHeaderCells();
	void UpdateModeButtonHighlight();
	void UpdateStatusText(const FText& Text, const FLinearColor& Color);
	bool WriteBackRow();
	UDataTable* GetTableForMode(EOnsetItemEditorMode Mode) const;
	const FOnsetItemDefinition* FindItemBase(EOnsetItemCategory Category, FName RowName) const;

	/** Validates every row in the given table (loot entries, sub-tables, scroll abilities). Returns empty on success. */
	FString ValidateTable(const UDataTable* Table) const;

	void HandlePropertyEdited();
	void PersistOnClose();

	UPROPERTY(Transient)
	TObjectPtr<UOnsetEquipmentRowWrapper> EquipmentWrapper;

	UPROPERTY(Transient)
	TObjectPtr<UOnsetQuestItemRowWrapper> QuestItemWrapper;

	UPROPERTY(Transient)
	TObjectPtr<UOnsetJunkRowWrapper> JunkWrapper;

	UPROPERTY(Transient)
	TObjectPtr<UOnsetScrollRowWrapper> ScrollWrapper;

	UPROPERTY(Transient)
	TObjectPtr<UOnsetLootRowWrapper> LootWrapper;

	EOnsetItemEditorMode CurrentMode = EOnsetItemEditorMode::Equipment;
	FName SelectedRowName;

	/** Modes whose tables differ from disk; persisted in PersistOnClose. */
	TSet<EOnsetItemEditorMode> DirtyModes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UOnsetItemModeButton>> ModeButtons;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> RowListBox;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UOnsetAbilityRowButton>> RowButtons;

	UPROPERTY(Transient)
	TObjectPtr<UOnsetNotifyDetailsView> PropertyView;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;
};