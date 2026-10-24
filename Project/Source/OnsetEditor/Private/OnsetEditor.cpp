// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnsetEditor.h"

#include "OnsetEditorModule.h"

const FName FOnsetEditorModule::AbilityEditorTabID = TEXT("OnsetAbilityEditor");
const FName FOnsetEditorModule::EnemyEditorTabID = TEXT("OnsetEnemyEditor");
const FName FOnsetEditorModule::ItemEditorTabID = TEXT("OnsetItemEditor");
const FName FOnsetEditorModule::ProgressionEditorTabID = TEXT("OnsetProgressionEditor");
const FName FOnsetEditorModule::QuestEditorTabID = TEXT("OnsetQuestEditor");
const FName FOnsetEditorModule::AppearanceStudioTabID = TEXT("OnsetAppearanceStudio");

IMPLEMENT_MODULE(FOnsetEditorModule, OnsetEditor)
DEFINE_LOG_CATEGORY(LogOnsetEditor);
