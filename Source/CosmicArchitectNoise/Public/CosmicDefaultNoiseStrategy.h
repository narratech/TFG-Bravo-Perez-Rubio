// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "ICosmicNoiseStrategy.h"
#include "CosmicNoiseTypes.h"
#include "ThirdParty/FastNoiseLite.h"

/**
 * Hyper-fast procedural noise strategy for planetary surfaces.
 *
 * Combines minimal noise sampling (1-2 evaluations per vertex) with analytical geomorphology:
 * - Hypsometric ocean basin and continental shelf slicing.
 * - Analytical alpine mountain crests (zero extra octave cost).
 * - Analytical geological terrace steps.
 * - Planetary latitudinal gradient and altitudinal cooling lapse rate.
 */
class COSMICARCHITECTNOISE_API FCosmicDefaultNoiseStrategy : public ICosmicNoiseStrategy
{
public:
    virtual ~FCosmicDefaultNoiseStrategy() override = default;

    int32 Seed = 1337;
    FCosmicNoiseLayer LayerParameters;
    FCosmicNoiseBiomeParameters BiomeParameters;

    float SeaLevel = 0.0f;
    float OceanDepthScale = 0.6f;
    float ContinentScale = 1.0f;
    bool bEnableMountainRidges = true;
    float MountainSharpness = 2.5f;
    float MountainRidgeStrength = 0.85f;
    float TerraceSteps = 0.0f;
    float HeightNormalizationScale = 1.0f;

    void Initialize(
        int32 InSeed,
        const FCosmicNoiseLayer& InLayerParameters,
        const FCosmicNoiseBiomeParameters& InBiomeParameters,
        float InSeaLevel = 0.0f,
        float InOceanDepthScale = 0.6f,
        float InContinentScale = 1.0f,
        bool bInEnableMountainRidges = true,
        float InMountainSharpness = 2.5f,
        float InMountainRidgeStrength = 0.85f,
        float InTerraceSteps = 0.0f,
        float InHeightNormalizationScale = 1.0f
    );

    virtual void EvaluatePoint(
        const FVector& NoiseDir,
        float& OutHeight,
        FLinearColor& OutColor
    ) const override;

protected:
    FastNoiseLite Noise;
    FastNoiseLite HumidityNoise;
};