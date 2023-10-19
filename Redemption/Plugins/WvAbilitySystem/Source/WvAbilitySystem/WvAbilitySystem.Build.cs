// Copyright 2020 wevet works All Rights Reserved.

using UnrealBuildTool;

public class WvAbilitySystem : ModuleRules
{
	public WvAbilitySystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bLegacyPublicIncludePaths = true;
		bUseUnity = false;
		bEnableUndefinedIdentifierWarnings = false;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		PublicIncludePaths.AddRange(
			new string[] 
			{
				"../../Engine/Plugins/Cameras/GameplayCameras/Source/GameplayCameras"
			}
		);
				
		
		PrivateIncludePaths.AddRange(
			new string[] 
			{
				// ... add other private include paths required here ...
			}
		);
					
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				// ... add other public dependencies that you statically link with here ...
				"Core",
				"GameplayTags",
                "GameplayTasks",
                "GameplayAbilities",
				"NetCore",
				"GameplayCameras",
				"AIModule",
			}
		);

		if (Target.Type == TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"UnrealEd",
			});
		}

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				// ... add private dependencies that you statically link with here ...	
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
			}
		);	
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);
	}
}
