#include "OnsetEditorModule.h"
#include "Misc/CoreDelegates.h"
#include "Modules/ModuleManager.h"

#include "LevelEditor.h"
#include "PropertyEditorModule.h"
#include "UI/OnsetAbilityEditorWidget.h"
#include "UI/OnsetAppearanceStudioWidget.h"
#include "UI/OnsetEnemyEditorWidget.h"
#include "UI/OnsetItemEditorWidget.h"
#include "UI/OnsetProgressionEditorWidget.h"
#include "UI/OnsetQuestEditorWidget.h"
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
    UOnsetQuestEditorWidget::EnsureAllTablesSeededAndSaved();
    UOnsetAppearanceStudioWidget::EnsureStarterAssetsSeeded();
}

void FOnsetEditorModule::ShutdownModule()
{
    if (IsRunningCommandlet()) return;
    FCoreDelegates::GetOnPostEngineInit().Remove(EngineReadyHandle);
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AbilityEditorTabID);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AppearanceStudioTabID);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(EnemyEditorTabID);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ItemEditorTabID);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ProgressionEditorTabID);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(QuestEditorTabID);

	UOnsetAppearanceStudioWidget::CleanupAllPreviewActors();

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

void FOnsetEditorModule::OpenAppearanceStudio()
{
	UOnsetAppearanceStudioWidget::EnsureStarterAssetsSeeded();
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			AppearanceStudioTabID,
			FOnSpawnTab::CreateRaw(this, &FOnsetEditorModule::SpawnAppearanceStudioTab));
	FGlobalTabmanager::Get()->TryInvokeTab(AppearanceStudioTabID);
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

void FOnsetEditorModule::OpenQuestEditor()
{
	UOnsetQuestEditorWidget::EnsureAllTablesSeededAndSaved();
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			QuestEditorTabID,
			FOnSpawnTab::CreateRaw(this, &FOnsetEditorModule::SpawnQuestEditorTab));
	FGlobalTabmanager::Get()->TryInvokeTab(QuestEditorTabID);
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
		FText::FromString("Appearance & Animation Studio"),
		FText::FromString("Author, preview, duplicate, and configure modular character appearances and animation profiles"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "DeveloperTools.MenuIcon"),
		FExecuteAction::CreateRaw(this, &FOnsetEditorModule::OpenAppearanceStudio)
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

	MenuBuilder.AddMenuEntry(
		FText::FromString("Quest & Dialogue Authoring"),
		FText::FromString("Author and edit Quests, NPCs, and Branching Dialogue trees"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "DeveloperTools.MenuIcon"),
		FExecuteAction::CreateRaw(this, &FOnsetEditorModule::OpenQuestEditor)
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

	Widget->OpenEditor();

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			Widget->TakeWidget()
		];
}

TSharedRef<SDockTab> FOnsetEditorModule::SpawnAppearanceStudioTab(const FSpawnTabArgs& Args)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		return SNew(SDockTab)
			.TabRole(NomadTab)
			[
				SNew(STextBlock).Text(NSLOCTEXT("OnsetEditor", "NoWorldStudio", "Open a level first to use the Appearance & Animation Studio."))
			];
	}

	UOnsetAppearanceStudioWidget* Widget = CreateWidget<UOnsetAppearanceStudioWidget>(World);
	if (!Widget)
	{
		return SNew(SDockTab)
			.TabRole(NomadTab)
			[
				SNew(STextBlock).Text(NSLOCTEXT("OnsetEditor", "CreateFailedStudio", "Failed to create the Appearance Studio widget."))
			];
	}

	Widget->OpenEditor();

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			Widget->TakeWidget()
		];

	SpawnedTab->SetOnTabClosed(SDockTab::FOnTabClosedCallback::CreateLambda([](TSharedRef<SDockTab>)
	{
		UOnsetAppearanceStudioWidget::CleanupAllPreviewActors();
	}));

	return SpawnedTab;
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
				SNew(STextBlock).Text(NSLOCTEXT("OnsetEditor", "NoWorldItem", "Open a level first to use the Item Editor."))
			];
	}

	UOnsetItemEditorWidget* Widget = CreateWidget<UOnsetItemEditorWidget>(World);
	if (!Widget)
	{
		return SNew(SDockTab)
			.TabRole(NomadTab)
			[
				SNew(STextBlock).Text(NSLOCTEXT("OnsetEditor", "CreateFailedItem", "Failed to create the Item Editor widget."))
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
				SNew(STextBlock).Text(NSLOCTEXT("OnsetEditor", "NoWorldProg", "Open a level first to use the Progression Editor."))
			];
	}

	UOnsetProgressionEditorWidget* Widget = CreateWidget<UOnsetProgressionEditorWidget>(World);
	if (!Widget)
	{
		return SNew(SDockTab)
			.TabRole(NomadTab)
			[
				SNew(STextBlock).Text(NSLOCTEXT("OnsetEditor", "CreateFailedProg", "Failed to create the Progression Editor widget."))
			];
	}

	Widget->OpenEditor();

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			Widget->TakeWidget()
		];
}

TSharedRef<SDockTab> FOnsetEditorModule::SpawnQuestEditorTab(const FSpawnTabArgs& Args)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		return SNew(SDockTab)
			.TabRole(NomadTab)
			[
				SNew(STextBlock).Text(NSLOCTEXT("OnsetEditor", "NoWorldQuest", "Open a level first to use the Quest & Dialogue Editor."))
			];
	}

	UOnsetQuestEditorWidget* Widget = CreateWidget<UOnsetQuestEditorWidget>(World);
	if (!Widget)
	{
		return SNew(SDockTab)
			.TabRole(NomadTab)
			[
				SNew(STextBlock).Text(NSLOCTEXT("OnsetEditor", "CreateFailedQuest", "Failed to create the Quest & Dialogue Editor widget."))
			];
	}

	Widget->OpenEditor();

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			Widget->TakeWidget()
		];
}
