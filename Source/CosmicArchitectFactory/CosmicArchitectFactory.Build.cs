// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CosmicArchitectFactory : ModuleRules
{
	public CosmicArchitectFactory(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
			
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
				"CoreUObject",
				"Engine",
				"CosmicArchitectRuntime",
				"CosmicArchitectNoise",
				"AssetTools",
                "AssetDefinition",
                "CosmicArchitectNoise",
                "CosmicArchitectFoliage"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "UnrealEd"
			}
			);
		
	}
}
