using UnrealBuildTool;

public class MHOnlineSteam : ModuleRules
{
    public MHOnlineSteam(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new string[] { "Core", "MHOnlineCore" });
    }
}
