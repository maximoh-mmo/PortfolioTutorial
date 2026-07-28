using UnrealBuildTool;
using System.IO;

public class OnsetDataStore : ModuleRules
{
	public OnsetDataStore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject"
		});

		if (Target.Type != TargetType.Client)
		{
			PrivateDependencyModuleNames.Add("HTTP");
			PrivateDependencyModuleNames.Add("Json");
			PrivateDependencyModuleNames.Add("SQLiteCore");

			string PostgresPath = Path.Combine(ModuleDirectory, "..", "Onset", "ThirdParty", "PostgreSQL");

			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				PublicIncludePaths.Add(Path.Combine(PostgresPath, "include"));
				PublicAdditionalLibraries.Add(Path.Combine(PostgresPath, "lib", "libpq.lib"));

				string BinariesDir = Path.Combine(PostgresPath, "bin");
				foreach (string DllFile in Directory.GetFiles(BinariesDir, "*.dll"))
				{
					RuntimeDependencies.Add(Path.Combine("$(BinaryOutputDir)", Path.GetFileName(DllFile)), DllFile);
				}
			}
			else if (Target.Platform == UnrealTargetPlatform.Linux)
			{
				PublicSystemIncludePaths.Add("/usr/include/postgresql");
				PublicSystemLibraries.Add("pq");
			}
		}
		else
		{
			PrivateDefinitions.Add("ONSETDATASTORE_CLIENT_ONLY");
		}
	}
}
