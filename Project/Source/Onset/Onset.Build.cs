// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Onset : ModuleRules
{
	public Onset(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", 
			"EnhancedInput", "AIModule", "UMG", "NavigationSystem", "StateTreeModule", "GameplayStateTreeModule",
			"GameplayAbilities", "GameplayTags", "GameplayTasks"});

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		PrivateDependencyModuleNames.AddRange(new string[] { "SlateCore" });
		
		//PrivateDependencyModuleNames.AddRange(new string[] { "Slate" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
