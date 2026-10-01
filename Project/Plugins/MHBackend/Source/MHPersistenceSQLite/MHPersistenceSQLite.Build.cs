using UnrealBuildTool;

public class MHPersistenceSQLite : ModuleRules
{
    public MHPersistenceSQLite(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "MHCore", "MHPersistence" });
        PrivateDependencyModuleNames.Add("SQLiteCore");
    }
}
