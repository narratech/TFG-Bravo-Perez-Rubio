// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "ICosmicNoiseStrategy.h"
#include "ThirdParty/FastNoiseLite.h"

/**
 * Standard procedural noise strategy used to
 * generate planetary surfaces with dynamic biomes.
 *
 * This implementation combines:
 * - Procedural base noise.
 * - Humidity variations.
 * - Temperature variations.
 * - Biome influence on final height.
 */
class COSMICARCHITECTNOISE_API FCosmicDefaultNoiseStrategy : public ICosmicNoiseStrategy
{
public:

    /**
     * Seed used to initialize all noise generators.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    int32 Seed;

    /**
     * General parameters of the main noise layer.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseLayer LayerParameters;

    /**
     * Parameters used for biome generation.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseBiomeParameters BiomeParameters;

    /**
     * Initializes the noise strategy and configures all
     * necessary internal generators.
     *
     * @param Seed Procedural seed used for noises.
     * @param LayerParameters Base noise configuration.
     * @param BiomeParameters Biome configuration.
     */
    void Initialize(
        int32 Seed,
        FCosmicNoiseLayer LayerParameters,
        FCosmicNoiseBiomeParameters BiomeParameters
    );

    /**
     * Evaluates a point on the procedural surface and calculates:
     * - Final height.
     * - Representative biome color.
     *
     * @param NoiseDir Normalized direction used as noise coordinate.
     * @param OutHeight Calculated resultant height.
     * @param OutColor Color associated with the generated biome.
     */
    void EvaluatePoint(
        const FVector& NoiseDir,
        float& OutHeight,
        FLinearColor& OutColor
    ) const override;

protected:

    /**
     * Noise generator used to calculate humidity.
     */
    FastNoiseLite HumidityNoise;

    /**
     * Noise generator used to calculate temperature.
     */
    FastNoiseLite TempNoise;

    /**
     * Main terrain base noise generator.
     */
    FastNoiseLite Noise;
};