using UnrealBuildTool;

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
			PrivateDependencyModuleNames.Add("SQLiteCore");
		}
		else
		{
			PrivateDefinitions.Add("ONSETDATASTORE_CLIENT_ONLY");
		}
	}
}
