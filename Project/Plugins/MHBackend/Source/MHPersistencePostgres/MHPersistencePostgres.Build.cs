using UnrealBuildTool;
using System;
using System.IO;

public class MHPersistencePostgres : ModuleRules
{
    public MHPersistencePostgres(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "MHCore", "MHPersistence" });
        // The host supplies an SDK; never reach into a game's source tree.
        string Root = Environment.GetEnvironmentVariable("MH_POSTGRES_ROOT");
        bool Enabled = Target.Type != TargetType.Client && !String.IsNullOrWhiteSpace(Root)
            && (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Linux);
        PrivateDefinitions.Add("WITH_MH_POSTGRES=" + (Enabled ? "1" : "0"));
        if (!Enabled) { return; }
        Root = Path.GetFullPath(Root);
        if (!File.Exists(Path.Combine(Root, "include", "libpq-fe.h")))
            throw new BuildException("MH_POSTGRES_ROOT must contain include/libpq-fe.h and the libpq library.");
        PrivateIncludePaths.Add(Path.Combine(Root, "include"));
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicAdditionalLibraries.Add(Path.Combine(Root, "lib", "libpq.lib"));
            foreach (string Dll in Directory.GetFiles(Path.Combine(Root, "bin"), "*.dll"))
                RuntimeDependencies.Add(Path.Combine("$(BinaryOutputDir)", Path.GetFileName(Dll)), Dll);
        }
        else
        {
            PublicAdditionalLibraries.Add(Path.Combine(Root, "lib", "libpq.so"));
            // Linux hosts must supply matching runtime libpq and transitive dependencies.
        }
    }
}
