// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CosmicArchitectBenchmark : ModuleRules
{
    public CosmicArchitectBenchmark(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        if (Target.Configuration == UnrealTargetConfiguration.Shipping)
        {
            // No compilar en Shipping
            return; 
        }


        PublicDependencyModuleNames.AddRange(
            new string[]
            {        
                "Core",
                "CosmicArchitectRuntime",
                "CosmicArchitectFoliage",
                "CosmicArchitectNoise",
                "RHI",
                "RenderCore"
            }
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
				"CoreUObject",
                "Engine",
                "Slate",
                "SlateCore"
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