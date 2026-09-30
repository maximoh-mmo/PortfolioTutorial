using UnrealBuildTool;

public class MHPersistence : ModuleRules
{
    public MHPersistence(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new string[] { "Core", "MHCore" });
    }
}
