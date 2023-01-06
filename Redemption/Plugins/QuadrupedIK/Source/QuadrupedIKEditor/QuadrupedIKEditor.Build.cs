using UnrealBuildTool;
using System;
using System.IO;

public class QuadrupedIKEditor : ModuleRules
{
	public QuadrupedIKEditor(ReadOnlyTargetRules Target) : base(Target)
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
				Path.Combine(ModuleDirectory,"Private")
			}
		);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"QuadrupedIK",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"AnimGraph", 
				"BlueprintGraph", 
				"Persona", 
				"UnrealEd", 
				"AnimGraphRuntime", 
				"SlateCore"
			}
		);


		BuildVersion Version;
		if (BuildVersion.TryRead(BuildVersion.GetDefaultFileName(), out Version))
		{
			if (Version.MajorVersion == 5)
			{
				PrivateDependencyModuleNames.AddRange(new string[] { "EditorFramework" });

				// From UE5.1, BaseClass of EditMode move to new Module 
				if (Version.MinorVersion >= 1)
				{
					PrivateDependencyModuleNames.AddRange(new string[] { "AnimationEditMode" });
				}
			}
		}

	}
}
