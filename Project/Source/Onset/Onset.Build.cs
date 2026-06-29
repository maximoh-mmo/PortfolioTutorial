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

		PrivateDependencyModuleNames.AddRange(new string[] { "SlateCore", "AITestSuite" });

		// Online features
		PrivateDependencyModuleNames.Add("OnlineSubsystem");
		PrivateDependencyModuleNames.Add("OnlineSubsystemSteam");

		// SQLite persistence
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "ThirdParty", "SQLite"));
		PrivateDefinitions.Add("SQLITE_THREADSAFE=1");
		PrivateDefinitions.Add("SQLITE_OMIT_LOAD_EXTENSION=1");
	}
}
	