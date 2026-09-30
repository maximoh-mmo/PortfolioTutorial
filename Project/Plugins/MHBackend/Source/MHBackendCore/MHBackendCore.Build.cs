using UnrealBuildTool;

public class MHBackendCore : ModuleRules
{
    public MHBackendCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "MHCore" });
        AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenSSL");
    }
}
