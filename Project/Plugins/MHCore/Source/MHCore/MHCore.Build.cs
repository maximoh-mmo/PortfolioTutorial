using UnrealBuildTool;

public class MHCore : ModuleRules
{
    public MHCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new string[] { "Core" });
    }
}
