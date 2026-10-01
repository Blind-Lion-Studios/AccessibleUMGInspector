// Copyright Blind Lion Studios. All Rights Reserved.

using UnrealBuildTool;

public class AccessibleUMGInspectorTests : ModuleRules
{
	public AccessibleUMGInspectorTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"AccessibleUMGInspector"
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
				"ToolsetRegistry",
				"Json",
				"JsonUtilities"
			}
		);
	}
}
