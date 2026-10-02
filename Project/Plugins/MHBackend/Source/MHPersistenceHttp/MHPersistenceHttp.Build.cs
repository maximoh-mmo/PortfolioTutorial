using UnrealBuildTool;

public class MHPersistenceHttp : ModuleRules
{
    public MHPersistenceHttp(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "MHCore", "MHPersistence" });
        PrivateDependencyModuleNames.Add("MHBackendHttp");
    }
}
