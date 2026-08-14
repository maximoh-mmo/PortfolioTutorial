#pragma once

#include "Editor.h"
#include "ToolMenus.h"
#include "Framework/Docking/TabManager.h"
#include "Modules/ModuleInterface.h"
#include "Widgets/Docking/SDockTab.h"

class UOnsetAbilityEditorWidget;

class FOnsetEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void RegisterMenus();
	void OpenAbilityEditor();
	
	void AddMenu(FMenuBarBuilder& MenuBarBuilder);
	void FillMenu(FMenuBuilder& MenuBuilder);
	
	TSharedRef<SDockTab> SpawnAbilityEditorTab(const FSpawnTabArgs& Args);

private:
	static const FName AbilityEditorTabID;

	/** Handle for the level actor-deleted subscription used to prune SpawnPoints. */
	FDelegateHandle OnLevelActorDeletedHandle;

	/** Handle for the actor-moved subscription used to flag manually-placed SpawnPoints. */
	FDelegateHandle OnActorMovedHandle;
};
