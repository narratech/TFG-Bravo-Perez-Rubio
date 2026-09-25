// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CosmicNoiseClass.h"
#include "CosmicDefaultNoiseSettings.generated.h"

/**
 * Configuration for the Hyper-Fast & Stylized procedural planetary noise strategy.
 *
 * Designed for extreme performance (only 1-2 noise evaluations per vertex)
 * with rich visual payoff:
 * - Analytical hypsometric oceans, shelves, and continental shields.
 * - Sharp alpine mountain ridges without extra octave cost.
 * - Analytical terraced geological strata.
 * - Latitudinal solar gradient and altitudinal cooling lapse rate.
 */
UCLASS()
class COSMICARCHITECTNOISE_API UCosmicDefaultNoiseSettings : public UCosmicNoiseClass
{
    GENERATED_BODY()

public:

    /** Base seed used for procedural generation */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    int32 Seed = 1337;

    /** General parameters of the primary macro noise layer */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseLayer LayerParameters;

    /** Parameters used for biome and climate generation */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseBiomeParameters BiomeParameters;

    /** Sea level threshold [-1.0, 1.0]. Terrain below this value forms oceans and basins. Default: 0.0 */
    UPROPERTY(EditAnywhere, Category = "Fast Planet - Continents & Oceans", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float SeaLevel = 0.0f;

    /** Depth scale multiplier for oceanic basins */
    UPROPERTY(EditAnywhere, Category = "Fast Planet - Continents & Oceans", meta = (ClampMin = "0.0", ClampMax = "3.0"))
    float OceanDepthScale = 0.6f;

    /** Elevation multiplier for continental landmasses */
    UPROPERTY(EditAnywhere, Category = "Fast Planet - Continents & Oceans", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float ContinentScale = 1.0f;

    /** Enable fast analytical mountain ridges without computing additional noise octaves */
    UPROPERTY(EditAnywhere, Category = "Fast Planet - Mountains")
    bool bEnableMountainRidges = true;

    /** Mountain ridge sharpness exponent. Higher values produce razor-sharp alpine ridges. */
    UPROPERTY(EditAnywhere, Category = "Fast Planet - Mountains", meta = (EditCondition = "bEnableMountainRidges", ClampMin = "0.5", ClampMax = "6.0"))
    float MountainSharpness = 2.5f;

    /** Strength of mountain ridges relative to base amplitude */
    UPROPERTY(EditAnywhere, Category = "Fast Planet - Mountains", meta = (EditCondition = "bEnableMountainRidges", ClampMin = "0.0", ClampMax = "3.0"))
    float MountainRidgeStrength = 0.85f;

    /** Terracing steps for stepped cliffs/plateaus (0.0 to disable) */
    UPROPERTY(EditAnywhere, Category = "Fast Planet - Mountains", meta = (ClampMin = "0.0", ClampMax = "25.0"))
    float TerraceSteps = 0.0f;

    /** Global terrain height normalization scale */
    UPROPERTY(EditAnywhere, Category = "Fast Planet - General", meta = (ClampMin = "0.01", ClampMax = "5.0"))
    float HeightNormalizationScale = 1.0f;

    /**
     * Creates and initializes the fast default noise strategy using current configuration.
     *
     * @return Fully initialized noise strategy.
     */
    virtual TSharedPtr<ICosmicNoiseStrategy> CreateStrategy() const override;
};