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
};
