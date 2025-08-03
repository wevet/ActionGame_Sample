// Copyright 2022 wevet works All Rights Reserved.

using UnrealBuildTool;


public class WvPostProcess : ModuleRules
{
	public WvPostProcess(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] 
			{
				// ... add public include paths required here ...
			}
		);


		PrivateIncludePaths.AddRange(
			new string[] 
			{
				// Needed to include the engine Lens Flare post-process header
				EngineDirectory + "/Source/Runtime/Renderer/Internal",
				EngineDirectory + "/Source/Runtime/Renderer/Private",
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				// Needed for RenderGraph, PostProcess, Shaders
				"Core",
				"CoreUObject",
				"RHI",
				"Renderer",
				"RenderCore",
				"Projects",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
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
