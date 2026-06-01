// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

using UnrealBuildTool;

public class CosmicArchitectEditor : ModuleRules
{
	public CosmicArchitectEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"CosmicArchitectRuntime",
				"CosmicArchitectNoise",
                "CosmicArchitectFoliage",
                "CosmicArchitectFactory",
                "CosmicArchitectCommon",
				"AssetTools",
				"AssetDefinition"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "LevelEditor",
                "UnrealEd"
			}
			);
		
	}
}
