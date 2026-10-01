// Copyright Blind Lion Studios. All Rights Reserved.

using UnrealBuildTool;

public class AccessibleUMGInspector : ModuleRules
{
	public AccessibleUMGInspector(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"ToolsetRegistry",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UMG",
				"UMGEditor",
				"UnrealEd",
				"AssetRegistry",
				"Json",
				"JsonUtilities"
			}
		);
	}
}
