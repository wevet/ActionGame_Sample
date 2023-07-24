
using UnrealBuildTool;
using System;
public class PredictiveFootIKEditor : ModuleRules
{
	public PredictiveFootIKEditor(ReadOnlyTargetRules Target): base(Target)
	{

        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        
        PublicIncludePaths.AddRange(
			new string[] {
                System.IO.Path.Combine(ModuleDirectory,"Public")
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
                System.IO.Path.Combine(ModuleDirectory,"Private")
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", 				
				"InputCore",
				"AnimGraph",
				"BlueprintGraph", 
				"UnrealEd",
				"Persona",
				"PredictiveFootIK",
			}
		);
        

        PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",                         
				"SlateCore",
				"ToolMenus",
			}
		);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}
