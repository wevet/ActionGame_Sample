// Copyright 2020 wevet works All Rights Reserved.

using UnrealBuildTool;

public class WvAbilitySystemEditor : ModuleRules
{
	public WvAbilitySystemEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		bUseUnity = false;

		bLegacyPublicIncludePaths = true;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		PublicIncludePaths.AddRange(
			new string[]
			{
				// ... add public include paths required here ...
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
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"UnrealEd",
				"Blutility",
				"ToolMenus",
				"LevelEditor",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"WvAbilitySystem",
				"AssetRegistry",
				"PropertyEditor",
				"EditorStyle",
				"KismetCompiler",
				"EditorSubsystem",
				"EditorWidgets",
				"UMGEditor",
				"UMG",
				"ContentBrowser",
				"ContentBrowserData",
				"BlueprintEditorLibrary",
				//"EditorScriptingUtilities",
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
