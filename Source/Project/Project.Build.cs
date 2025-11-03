// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Project : ModuleRules
{
	public Project(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"Project",
			"Project/Variant_Platforming",
			"Project/Variant_Platforming/Animation",
			"Project/Variant_Combat",
			"Project/Variant_Combat/AI",
			"Project/Variant_Combat/Animation",
			"Project/Variant_Combat/Gameplay",
			"Project/Variant_Combat/Interfaces",
			"Project/Variant_Combat/UI",
			"Project/Variant_SideScrolling",
			"Project/Variant_SideScrolling/AI",
			"Project/Variant_SideScrolling/Gameplay",
			"Project/Variant_SideScrolling/Interfaces",
			"Project/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
