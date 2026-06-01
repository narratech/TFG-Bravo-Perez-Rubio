// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

using UnrealBuildTool;

public class CosmicArchitectCommon : ModuleRules
{
	public CosmicArchitectCommon(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
             "Core",
             "CoreUObject"
        });
    }
}
 