using UnrealBuildTool;

public class MHOnlineCore : ModuleRules
{
    public MHOnlineCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new string[] { "Core", "MHCore" });
    }
}
