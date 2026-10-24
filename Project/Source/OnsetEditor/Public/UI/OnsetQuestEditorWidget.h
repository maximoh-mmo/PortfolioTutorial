// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "EditorUtilityWidget.h"
#include "Data/OnsetDialogueTypes.h"
#include "Data/OnsetQuestTypes.h"
#include "OnsetQuestEditorWidget.generated.h"

class UDataTable;
class UVerticalBox;
class UHorizontalBox;
class UTextBlock;
class USizeBox;
class UOnsetAbilityRowButton;
class UOnsetNotifyDetailsView;

/** Editor mode = one row struct + one DataTable. */
UENUM()
enum class EOnsetQuestEditorMode : uint8
{
	Quests,
	NPCs,
	Dialogues
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnQuestModeSelected, EOnsetQuestEditorMode);

/** Mode-tab button; carries its mode and broadcasts selection. */
UCLASS()
class UOnsetQuestModeButton : public UButton
{
	GENERATED_BODY()

public:
	EOnsetQuestEditorMode Mode = EOnsetQuestEditorMode::Quests;
	FOnQuestModeSelected OnModeSelected;

	UFUNCTION()
	void HandleClicked() { OnModeSelected.Broadcast(Mode); }
};

/** Transient wrapper exposing a single DT_QuestDefinition row to the PropertyView. */
UCLASS(Transient)
class UOnsetQuestRowWrapper : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FOnsetQuestDefinition Row;
};

/** Transient wrapper exposing a single DT_NPCs row to the PropertyView. */
UCLASS(Transient)
class UOnsetNPCRowWrapper : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FOnsetNPCDefinition Row;
};

/** Transient wrapper exposing a single DT_Dialogues row to the PropertyView. */
UCLASS(Transient)
class UOnsetDialogueRowWrapper : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FOnsetDialogueRow Row;
};

/**
 * Editor Utility Widget for in-editor quest authoring, NPC configuration, and
 * branching dialogue editing. Modeled after UOnsetItemEditorWidget.
 *
 * Top: mode tabs (Quests / NPCs / Dialogues).
 * Left: row list with customized status columns.
 * Right: PropertyView bound to the selected row wrapper with immediate write-back.
 * Bottom: Add, Delete, Validate, and Save actions with live validation diagnostics.
 */
UCLASS()
class ONSETEDITOR_API UOnsetQuestEditorWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Quest Editor")
	void OpenEditor();

	UFUNCTION(BlueprintCallable, Category = "Quest Editor")
	void RefreshFromTable();

	UFUNCTION(BlueprintCallable, Category = "Quest Editor")
	void AddDefinition();

	UFUNCTION(BlueprintCallable, Category = "Quest Editor")
	void DeleteDefinition();

	UFUNCTION(BlueprintCallable, Category = "Quest Editor")
	void ValidateAll();

	UFUNCTION(BlueprintCallable, Category = "Quest Editor")
	void SaveAll();

	/** Switches the editor to the given mode and reloads the list. */
	UFUNCTION(BlueprintCallable, Category = "Quest Editor")
	void SetMode(EOnsetQuestEditorMode NewMode);

	/** Switches to the mode carried by the clicked tab button. */
	UFUNCTION()
	void OnModeButtonClicked(EOnsetQuestEditorMode NewMode) { SetMode(NewMode); }

	void SelectRow(FName RowName);

	/** Ensures DT_QuestDefinition, DT_NPCs, and DT_Dialogues exist, are seeded with defaults if empty, and saved to disk. */
	static void EnsureAllTablesSeededAndSaved();
	static UDataTable* GetTableForMode(EOnsetQuestEditorMode Mode);
	static bool EnsureDefaultRowsForTable(EOnsetQuestEditorMode Mode, UDataTable* Table);

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

	FString ValidateTable(EOnsetQuestEditorMode Mode, const UDataTable* Table) const;

	void HandlePropertyEdited();
	void PersistOnClose();

	UPROPERTY(Transient)
	TObjectPtr<UOnsetQuestRowWrapper> QuestWrapper;

	UPROPERTY(Transient)
	TObjectPtr<UOnsetNPCRowWrapper> NPCWrapper;

	UPROPERTY(Transient)
	TObjectPtr<UOnsetDialogueRowWrapper> DialogueWrapper;

	EOnsetQuestEditorMode CurrentMode = EOnsetQuestEditorMode::Quests;
	FName SelectedRowName;

	/** Modes whose tables differ from disk; persisted in PersistOnClose. */
	TSet<EOnsetQuestEditorMode> DirtyModes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UOnsetQuestModeButton>> ModeButtons;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> HeaderRow;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> RowListBox;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UOnsetAbilityRowButton>> RowButtons;

	UPROPERTY(Transient)
	TObjectPtr<UOnsetNotifyDetailsView> PropertyView;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ModeInfoText;
};
