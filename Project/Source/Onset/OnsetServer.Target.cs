// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class OnsetServerTarget : TargetRules
{
	public OnsetServerTarget(TargetInfo Target) : base(Target)
	{
		Type = UnrealBuildTool.TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("Onset");
	}
}