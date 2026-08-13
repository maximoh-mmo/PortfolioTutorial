#include "OnsetEditorModule.h"

#include "LevelEditor.h"
#include "OnsetAbilityEditorWidget.h"
#include "Blueprint/UserWidget.h"

void FOnsetEditorModule::StartupModule()
{	
	
	UToolMenus::RegisterStartupCallback(
				FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FOnsetEditorModule::RegisterMenus));
}

void FOnsetEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AbilityEditorTabID);
}

void FOnsetEditorModule::RegisterMenus()
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	const TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender);
	MenuExtender->AddMenuBarExtension(
		"Help",
		EExtensionHook::After,
		nullptr,
		FMenuBarExtensionDelegate::CreateRaw(this, &FOnsetEditorModule::AddMenu)
		);
	LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
}

void FOnsetEditorModule::OpenAbilityEditor()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			AbilityEditorTabID,
			FOnSpawnTab::CreateRaw(this, &FOnsetEditorModule::SpawnAbilityEditorTab));
	FGlobalTabmanager::Get()->TryInvokeTab(AbilityEditorTabID);
}

void FOnsetEditorModule::AddMenu(FMenuBarBuilder& MenuBarBuilder)
{
	MenuBarBuilder.AddPullDownMenu(
		FText::FromString("Onset Tools"),
		FText::FromString("Onset Editor Tools"),
		FNewMenuDelegate::CreateRaw(this, &FOnsetEditorModule::FillMenu)
		);
}

void FOnsetEditorModule::FillMenu(FMenuBuilder& MenuBuilder)
{
	FOnsetEditorModule* ctx(this);
	FUIAction delegate;
	delegate.ExecuteAction.BindRaw(this, &FOnsetEditorModule::OpenAbilityEditor);
	MenuBuilder.AddMenuEntry(
		FText::FromString("Onset Editor"),
		FText::FromString("Onset Editor Tool Tip"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "DeveloperTools.MenuIcon"),
		FExecuteAction::CreateRaw(this, &FOnsetEditorModule::OpenAbilityEditor)
		);		
}

TSharedRef<SDockTab> FOnsetEditorModule::SpawnAbilityEditorTab(const FSpawnTabArgs& Args)
{	
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		return SNew(SDockTab)
			.TabRole(NomadTab)
			[
				SNew(STextBlock).Text(NSLOCTEXT("OnsetEditor", "NoWorld", "Open a level first to use the Ability Editor."))
			];
	}

	UOnsetAbilityEditorWidget* Widget = CreateWidget<UOnsetAbilityEditorWidget>(World);
	if (!Widget)
	{
		return SNew(SDockTab)
			.TabRole(NomadTab)
			[
				SNew(STextBlock).Text(NSLOCTEXT("OnsetEditor", "CreateFailed", "Failed to create the Ability Editor widget."))
			];
	}

	// Build the widget tree BEFORE TakeWidget: the first TakeWidget snapshots
	// WidgetTree->RootWidget, and NativeConstruct fires too late (after the
	// surface is already cached as an empty SSpacer).
	Widget->OpenEditor();

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			Widget->TakeWidget()
		];
}
