#pragma once

#include "Editor.h"
#include "ToolMenus.h"
#include "Framework/Docking/TabManager.h"
#include "Modules/ModuleInterface.h"
#include "Widgets/Docking/SDockTab.h"

class UOnsetAbilityEditorWidget;
class UOnsetEnemyEditorWidget;
class UOnsetItemEditorWidget;
class UOnsetProgressionEditorWidget;

class FOnsetEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void RegisterMenus();
	void OpenAbilityEditor();
	void OpenEnemyEditor();
	void OpenItemEditor();
	void OpenProgressionEditor();
	
	void AddMenu(FMenuBarBuilder& MenuBarBuilder);
	void FillMenu(FMenuBuilder& MenuBuilder);
	
	TSharedRef<SDockTab> SpawnAbilityEditorTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnEnemyEditorTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnItemEditorTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnProgressionEditorTab(const FSpawnTabArgs& Args);

private:
    void HandleEngineReady();
    FDelegateHandle EngineReadyHandle;
	static const FName AbilityEditorTabID;
	static const FName EnemyEditorTabID;
	static const FName ItemEditorTabID;
	static const FName ProgressionEditorTabID;

	/** Handle for the level actor-deleted subscription used to prune SpawnPoints. */
	FDelegateHandle OnLevelActorDeletedHandle;

	/** Handle for the actor-moved subscription used to flag manually-placed SpawnPoints. */
	FDelegateHandle OnActorMovedHandle;
};
