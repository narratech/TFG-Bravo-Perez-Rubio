// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CosmicArchitectRuntime : ModuleRules
{
    public CosmicArchitectRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
				// ... add public include paths required here ...
			}
            );


        PrivateIncludePaths.AddRange(
            new string[] {
				// ... add other private include paths required here ...
			}
            );


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
                "RHI",        
                "RenderCore",    
			}
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
				// ... add private dependencies that you statically link with here ...
				"Slate",
                "SlateCore",
            }
            );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
            );

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd" });
        }

        // CONFIGURACIÓN ADICIONAL PARA BENCHMARK
        // Habilitar stats (excepto en Shipping)
        if (Target.Configuration != UnrealTargetConfiguration.Shipping)
        {
            PrivateDefinitions.Add("STATS=1");
            PrivateDefinitions.Add("CSV_PROFILER=1");
        }
    }
}