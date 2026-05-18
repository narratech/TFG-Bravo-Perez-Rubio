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
            }
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
				"CoreUObject",
                "Engine",
            }
            );
    }
}