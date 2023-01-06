
using UnrealBuildTool;
using System;
using System.IO;

public class QuadrupedIK : ModuleRules
{
	public QuadrupedIK(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[]
			{
				Path.Combine(ModuleDirectory,"Public")
			}
		);


		PrivateIncludePaths.AddRange(
			new string[] 
			{
				Path.Combine(ModuleDirectory,"Private"),
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
				"AnimGraphRuntime",
				"AnimationCore",
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
		);


	}
}

