// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class Onset : ModuleRules
{
	public Onset(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", 
			"EnhancedInput", "AIModule", "UMG", "NavigationSystem", "StateTreeModule", "GameplayStateTreeModule",
			"GameplayAbilities", "GameplayTags", "GameplayTasks"});

		PrivateDependencyModuleNames.AddRange(new string[] { "SlateCore", "AITestSuite", "CommonUI"});

		// Online features
		PrivateDependencyModuleNames.Add("OnlineSubsystem");
		PrivateDependencyModuleNames.Add("OnlineSubsystemSteam");

		// Data store (SQLite/PostgreSQL) — abstracted via IPlayerDataStore
		PublicDependencyModuleNames.Add("OnsetDataStore");
	}
}
	