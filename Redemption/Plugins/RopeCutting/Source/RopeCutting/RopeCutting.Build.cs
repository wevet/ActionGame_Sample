// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RopeCutting : ModuleRules
{
	public RopeCutting(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] 
			{
			}
		);


		PrivateIncludePaths.AddRange(
			new string[] 
			{
			}
		);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"PhysicsCore",
			}
		);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}
