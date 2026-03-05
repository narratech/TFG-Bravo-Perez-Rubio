// Copyright Epic Games, Inc. All Rights Reserved.

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
            "CosmicArchitectCommon"  
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
