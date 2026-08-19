// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnsetEditor.h"

#include "OnsetEditorModule.h"

const FName FOnsetEditorModule::AbilityEditorTabID = TEXT("OnsetAbilityEditor");
const FName FOnsetEditorModule::EnemyEditorTabID = TEXT("OnsetEnemyEditor");
const FName FOnsetEditorModule::ItemEditorTabID = TEXT("OnsetItemEditor");

IMPLEMENT_MODULE(FOnsetEditorModule, OnsetEditor)
DEFINE_LOG_CATEGORY(LogOnsetEditor);
