// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class Onset : ModuleRules
{
	public Onset(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "OnsetAppearance", "Engine", "InputCore",
			"EnhancedInput", "AIModule", "UMG", "NavigationSystem", "StateTreeModule", "GameplayStateTreeModule",
			"GameplayAbilities", "GameplayTags", "GameplayTasks", "CommonUI", "MHInteraction"});

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "AITestSuite", "Json"});

		// Online features
		PrivateDependencyModuleNames.Add("OnlineSubsystem");
		PrivateDependencyModuleNames.Add("OnlineSubsystemSteam");

		// Data store (SQLite/PostgreSQL) — abstracted via IPlayerDataStore
		PublicDependencyModuleNames.Add("OnsetDataStore");

		// Signing/verifying is server-side; clients only carry opaque session tokens.
		if (Target.Type != TargetType.Client)
		{
			PrivateDependencyModuleNames.Add("MHBackendCore");
		}
	}
}
