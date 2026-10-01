using UnrealBuildTool;

public class MHPersistence : ModuleRules
{
    public MHPersistence(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "MHCore" });
    }
}
