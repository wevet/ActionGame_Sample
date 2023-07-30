// Copyright 2022 wevet works All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class RedemptionTarget : TargetRules
{
	public RedemptionTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_1;
		ExtraModuleNames.Add("Redemption");
	}
}
