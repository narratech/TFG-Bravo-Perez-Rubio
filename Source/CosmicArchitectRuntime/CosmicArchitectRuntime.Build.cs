// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CosmicArchitectRuntime : ModuleRules
{
    public CosmicArchitectRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "CosmicArchitectCommon",
                "ProceduralMeshComponent",
                "CosmicArchitectNoise",
                "CosmicArchitectFoliage",
                "InputCore",
                "PhysicsCore",
                "EnhancedInput",  
			}
            );

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicSystemLibraries.Add("DXGI.lib");
            PublicSystemLibraries.Add("D3D11.lib");
        }

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd" });
        }
    }
}