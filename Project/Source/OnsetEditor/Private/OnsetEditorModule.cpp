#include "OnsetEditorModule.h"
#include "Misc/CoreDelegates.h"
#include "Modules/ModuleManager.h"

#include "LevelEditor.h"
#include "PropertyEditorModule.h"
#include "UI/OnsetAbilityEditorWidget.h"
#include "UI/OnsetEnemyEditorWidget.h"
#include "UI/OnsetItemEditorWidget.h"
#include "UI/OnsetProgressionEditorWidget.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "SpawnerEditorUtilities.h"
#include "Spawning/SpawnPoint.h"

/** Editor-time hook: dragging a spawn point marks it as manually placed so the
 *  spawner stops auto-relocating it. Does not fire for programmatic moves. */
static void OnSpawnPointMoved(AActor* Actor)
{
	if (ASpawnPoint* SpawnPoint = Cast<ASpawnPoint>(Actor))
	{
		SpawnPoint->bUserPlaced = true;
	}
}

void FOnsetEditorModule::StartupModule()
{
    if (IsRunningCommandlet()) return;
    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FOnsetEditorModule::RegisterMenus));
    EngineReadyHandle=FCoreDelegates::GetOnPostEngineInit().AddRaw(this,&FOnsetEditorModule::HandleEngineReady);
    if (GEngine) HandleEngineReady();
}
void FOnsetEditorModule::HandleEngineReady()
{
    if (!GEngine) return;
    if (!OnLevelActorDeletedHandle.IsValid()) OnLevelActorDeletedHandle=GEngine->OnLevelActorDeleted().AddStatic(&FSpawnerEditorUtilities::OnActorDeleted);
    if (!OnActorMovedHandle.IsValid()) OnActorMovedHandle=GEngine->OnActorMoved().AddStatic(&OnSpawnPointMoved);
}
void FOnsetEditorModule::ShutdownModule()
{
    if (IsRunningCommandlet()) return;
    FCoreDelegates::GetOnPostEngineInit().Remove(EngineReadyHandle);
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AbilityEditorTabID);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(EnemyEditorTabID);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ItemEditorTabID);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ProgressionEditorTabID);

	if (GEngine)
	{
		if (OnLevelActorDeletedHandle.IsValid())
		{
			GEngine->OnLevelActorDeleted().Remove(OnLevelActorDeletedHandle);
			OnLevelActorDeletedHandle.Reset();
		}

		if (OnActorMovedHandle.IsValid())
		{
			GEngine->OnActorMoved().Remove(OnActorMovedHandle);
			OnActorMovedHandle.Reset();
		}
	}
}

void FOnsetEditorModule::RegisterMenus()
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	const TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender);
	MenuExtender->AddMenuBarExtension(
		"Help",
		EExtensionHook::Before,
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

void FOnsetEditorModule::OpenEnemyEditor()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			EnemyEditorTabID,
			FOnSpawnTab::CreateRaw(this, &FOnsetEditorModule::SpawnEnemyEditorTab));
	FGlobalTabmanager::Get()->TryInvokeTab(EnemyEditorTabID);
}

void FOnsetEditorModule::OpenItemEditor()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			ItemEditorTabID,
			FOnSpawnTab::CreateRaw(this, &FOnsetEditorModule::SpawnItemEditorTab));
	FGlobalTabmanager::Get()->TryInvokeTab(ItemEditorTabID);
}

void FOnsetEditorModule::OpenProgressionEditor()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			ProgressionEditorTabID,
			FOnSpawnTab::CreateRaw(this, &FOnsetEditorModule::SpawnProgressionEditorTab));
	FGlobalTabmanager::Get()->TryInvokeTab(ProgressionEditorTabID);
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
	MenuBuilder.AddMenuEntry(
		FText::FromString("Ability Builder"),
		FText::FromString("Author Abilities, automatically updates DT_Abilities"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "DeveloperTools.MenuIcon"),
		FExecuteAction::CreateRaw(this, &FOnsetEditorModule::OpenAbilityEditor)
		);

	MenuBuilder.AddMenuEntry(
		FText::FromString("Enemy Editor"),
		FText::FromString("Author DT_EnemyStats rows with linked visual/AI/perception profiles"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "DeveloperTools.MenuIcon"),
		FExecuteAction::CreateRaw(this, &FOnsetEditorModule::OpenEnemyEditor)
		);

	MenuBuilder.AddMenuEntry(
		FText::FromString("Item & Loot Editor"),
		FText::FromString("Edit item category tables and DT_Loot with validation and roll preview"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "DeveloperTools.MenuIcon"),
		FExecuteAction::CreateRaw(this, &FOnsetEditorModule::OpenItemEditor)
		);

	MenuBuilder.AddMenuEntry(
		FText::FromString("Progression & Balance Workbench"),
		FText::FromString("Interlinked tuning for XP curves, combat TTK, equipment tiers, and loot drop rates"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "DeveloperTools.MenuIcon"),
		FExecuteAction::CreateRaw(this, &FOnsetEditorModule::OpenProgressionEditor)
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

TSharedRef<SDockTab> FOnsetEditorModule::SpawnEnemyEditorTab(const FSpawnTabArgs& Args)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		return SNew(SDockTab)
			.TabRole(NomadTab)
			[
				SNew(STextBlock).Text(NSLOCTEXT("OnsetEditor", "NoWorldEnemy", "Open a level first to use the Enemy Editor."))
			];
	}

	UOnsetEnemyEditorWidget* Widget = CreateWidget<UOnsetEnemyEditorWidget>(World);
	if (!Widget)
	{
		return SNew(SDockTab)
			.TabRole(NomadTab)
			[
				SNew(STextBlock).Text(NSLOCTEXT("OnsetEditor", "CreateFailedEnemy", "Failed to create the Enemy Editor widget."))
			];
	}

	Widget->OpenEditor();

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			Widget->TakeWidget()
		];
}

TSharedRef<SDockTab> FOnsetEditorModule::SpawnItemEditorTab(const FSpawnTabArgs& Args)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		return SNew(SDockTab)
			.TabRole(NomadTab)
			[
				SNew(STextBlock).Text(NSLOCTEXT("OnsetEditor", "NoWorldItem", "Open a level first to use the Item & Loot Editor."))
			];
	}

	UOnsetItemEditorWidget* Widget = CreateWidget<UOnsetItemEditorWidget>(World);
	if (!Widget)
	{
		return SNew(SDockTab)
			.TabRole(NomadTab)
			[
				SNew(STextBlock).Text(NSLOCTEXT("OnsetEditor", "CreateFailedItem", "Failed to create the Item & Loot Editor widget."))
			];
	}

	Widget->OpenEditor();

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			Widget->TakeWidget()
		];
}

TSharedRef<SDockTab> FOnsetEditorModule::SpawnProgressionEditorTab(const FSpawnTabArgs& Args)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		return SNew(SDockTab)
			.TabRole(NomadTab)
			[
				SNew(STextBlock).Text(NSLOCTEXT("OnsetEditor", "NoWorldProgression", "Open a level first to use the Progression & Balance Workbench."))
			];
	}

	UOnsetProgressionEditorWidget* Widget = CreateWidget<UOnsetProgressionEditorWidget>(World);
	if (!Widget)
	{
		return SNew(SDockTab)
			.TabRole(NomadTab)
			[
				SNew(STextBlock).Text(NSLOCTEXT("OnsetEditor", "CreateFailedProgression", "Failed to create the Progression & Balance Workbench widget."))
			];
	}

	Widget->OpenEditor();

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			Widget->TakeWidget()
		];
}
