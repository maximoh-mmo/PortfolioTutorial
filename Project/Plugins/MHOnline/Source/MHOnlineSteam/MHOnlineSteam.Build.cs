using UnrealBuildTool;

public class MHOnlineSteam : ModuleRules
{
    public MHOnlineSteam(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "MHOnlineCore" });
        PrivateDependencyModuleNames.Add("MHCore");
        // Dedicated servers use the Steam Game Server API. Keep the SDK available
        // there as well so the module can observe its callback lifecycle.
        bool NativeSteam = Target.Platform == UnrealTargetPlatform.Win64;
        PrivateDefinitions.Add("MH_STEAM_NATIVE=" + (NativeSteam ? "1" : "0"));
        if (NativeSteam)
        {
            PrivateDependencyModuleNames.AddRange(new string[] { "OnlineSubsystem", "SteamShared" });
            AddEngineThirdPartyPrivateStaticDependencies(Target, "Steamworks");
        }
    }
}
