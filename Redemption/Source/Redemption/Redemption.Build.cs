// Copyright 2022 wevet works All Rights Reserved.

using UnrealBuildTool;

public class Redemption : ModuleRules
{
	public Redemption(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bLegacyPublicIncludePaths = true;
		bUseUnity = false;
		bEnableUndefinedIdentifierWarnings = false;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreOnline",
				"CoreUObject",
				"ApplicationCore",
				"Engine",
				"InputCore",
				"EnhancedInput",
				"GameplayTags",
				"GameplayTasks",
				"GameplayAbilities",
				"AIModule",
				"PhysicsCore",
				"DataRegistry",
				"ReplicationGraph",
				"Json",
				//"AnimationLocomotionLibraryRuntime",
				"PoseSearch",
				"ControlRig",
				"MotionWarping",
				"MotionTrajectory",
				"ChaosVehiclesCore",
				"StateTreeModule",
				"StructUtils",
				"HairStrandsCore",
				"ContextualAnimation",
				"Media",
				"MediaUtils",
				"MediaAssets",
				"AudioMixer",
				"GeometryCollectionEngine",
				"Gauntlet",
				"MassZoneGraphNavigation",
				"Niagara",
				"QuadrupedIK",
				"RopeCutting",
			}
		);


		// For non-shipping build.
		if (Target.Configuration != UnrealTargetConfiguration.Shipping)
		{
		//	PublicDependencyModuleNames.AddRange
		//	(
		//		new string[]
		//		{
		//			"ImGui",
		//			"ApplicationCore",
		//		}
		//	);
		}


		//if (Target.Type == TargetType.Editor)
		//{
		//	PrivateDependencyModuleNames.AddRange
		//	(
		//		new string[]
		//		{
		//			"UnrealEnginePython",
		//			"UnrealEd",
		//			"SequenceRecorder",
		//		}
		//	);
		//}

		// Generate compile errors if using DrawDebug functions in test/shipping builds.
		PublicDefinitions.Add("SHIPPING_DRAW_DEBUG_ERROR=1");
	}
}

