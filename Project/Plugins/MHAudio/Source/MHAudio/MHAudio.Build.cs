// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MHAudio : ModuleRules
{
    public MHAudio(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "DeveloperSettings",
            "MHCore"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "AudioExtensions",
            "AudioPlatformConfiguration"
        });
    }
}
