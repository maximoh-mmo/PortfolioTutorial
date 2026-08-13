// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnsetEditor.h"

#include "OnsetEditorModule.h"

const FName FOnsetEditorModule::AbilityEditorTabID = TEXT("OnsetAbilityEditor");

IMPLEMENT_MODULE(FOnsetEditorModule, OnsetEditor)
DEFINE_LOG_CATEGORY(LogOnsetEditor);
