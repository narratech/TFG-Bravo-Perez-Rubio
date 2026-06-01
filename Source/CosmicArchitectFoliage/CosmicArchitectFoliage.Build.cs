// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

using UnrealBuildTool;

public class CosmicArchitectFoliage : ModuleRules
{
    public CosmicArchitectFoliage(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "CosmicArchitectCommon",
            "CosmicArchitectNoise"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "ProceduralMeshComponent",  
            "Foliage"                   
        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[] {
                "UnrealEd",
                "FoliageEdit"
            });
        }
    }
}
