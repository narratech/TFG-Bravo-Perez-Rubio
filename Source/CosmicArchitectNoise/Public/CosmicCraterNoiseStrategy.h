// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "ICosmicNoiseStrategy.h"
#include "ThirdParty/FastNoiseLite.h"

/**
 * Procedural noise generation strategy oriented
 * toward cratered planetary surfaces.
 *
 * This implementation combines:
 * - Procedural base noise.
 * - Biome generation.
 * - Humidity and temperature variations.
 * - Multilayer crater generation.
 */
class COSMICARCHITECTNOISE_API FCosmicCraterNoiseStrategy : public ICosmicNoiseStrategy
{
public:

    /**
     * Seed used to initialize all noise generators.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    int32 Seed;

    /**
     * General parameters of base noise layers.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseLayer LayerParameters;

    /**
     * Parameters related to biome generation.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseBiomeParameters BiomeParameters;

    /**
     * Specific parameters used for crater generation.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseCraterParameters CraterParameters;

    /**
     * Initializes the noise strategy and configures all
     * necessary internal generators.
     *
     * @param Seed Procedural seed used for noises.
     * @param LayerParameters Base noise configuration.
     * @param BiomeParameters Biome configuration.
     * @param CraterParameters Crater generation configuration.
     */
    void Initialize(
        int32 Seed,
        FCosmicNoiseLayer LayerParameters,
        FCosmicNoiseBiomeParameters BiomeParameters,
        FCosmicNoiseCraterParameters CraterParameters
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

    /**
     * Noise generator used for crater formation.
     */
    FastNoiseLite CraterNoise;
};