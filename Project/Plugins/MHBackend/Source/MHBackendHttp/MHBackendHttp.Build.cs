using UnrealBuildTool;

public class MHBackendHttp : ModuleRules
{
    public MHBackendHttp(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new string[] { "Core", "MHBackendCore" });
    }
}
