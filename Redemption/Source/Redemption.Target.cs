// Copyright 2022 wevet works All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class RedemptionTarget : TargetRules
{
	public RedemptionTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		// 異なるオブジェクトファイル間での最適化
		bAllowLTCG = true;

		if (Target.Configuration == UnrealTargetConfiguration.Shipping || Target.Configuration == UnrealTargetConfiguration.Test)
		{
			// 計測パッケージ作成時
			bPGOProfile = true;
			// PGO適用時
			//bPGOOptimize= true;
		}

		ExtraModuleNames.Add("Redemption");
	}
}
