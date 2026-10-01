using UnrealBuildTool;

public class MHBackendHttp : ModuleRules
{
    public MHBackendHttp(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "MHCore" });
        PrivateDependencyModuleNames.Add("HTTP");
    }
}
