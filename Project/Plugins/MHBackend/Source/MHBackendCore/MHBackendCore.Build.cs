using UnrealBuildTool;

public class MHBackendCore : ModuleRules
{
    public MHBackendCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new string[] { "Core", "MHCore" });
    }
}
