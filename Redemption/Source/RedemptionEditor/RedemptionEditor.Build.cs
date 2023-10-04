// Copyright 2022 wevet works All Rights Reserved.

using UnrealBuildTool;

public class RedemptionEditor : ModuleRules
{
	public RedemptionEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		bLegacyPublicIncludePaths = true;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		OverridePackageType = PackageOverrideType.GameUncookedOnly;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Slate",
				"SlateCore",
				"Engine",
				"UnrealEd",
				"PropertyEditor",
				"AnimGraphRuntime",
				"AnimGraph",
				"GraphEditor",
				"BlueprintGraph",
				"DesktopPlatform",
				"AnimationModifiers",
				"InputCore",
				"Redemption",
				"RHI",
				"Landscape",
				"LevelEditor",
				"PlacementMode",
				"DeveloperSettings",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"EditorStyle",
				"AssetRegistry",
				"Slate",
				"SlateCore",
				"AssetTools",
				"EditorStyle",
				"EditorScriptingUtilities"
			}
		);

	}
}
