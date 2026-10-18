using UnrealBuildTool;
public class OnsetAppearance : ModuleRules
{
    public OnsetAppearance(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
        PrivateDependencyModuleNames.AddRange(new[] { "Json", "JsonUtilities" });
    }
}
