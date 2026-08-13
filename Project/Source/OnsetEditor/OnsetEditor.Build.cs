// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class OnsetEditor : ModuleRules
{
	public OnsetEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Onset",
			"Blutility",
			"EditorScriptingUtilities",
			"UMG",
			"Slate",
			"SlateCore",
			"UnrealEd",
			"GameplayTags",
			"PropertyEditor",
			"ScriptableEditorWidgets",
			"ToolMenus",
			"WorkspaceMenuStructure"
		});
	}
}
