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

		// �قȂ�I�u�W�F�N�g�t�@�C���Ԃł̍œK��
		bAllowLTCG = true;

		if (Target.Configuration == UnrealTargetConfiguration.Shipping || Target.Configuration == UnrealTargetConfiguration.Test)
		{
			// �v���p�b�P�[�W�쐬��
			bPGOProfile = true;
			// PGO�K�p��
			//bPGOOptimize= true;
		}

		ExtraModuleNames.Add("Redemption");
	}
}
