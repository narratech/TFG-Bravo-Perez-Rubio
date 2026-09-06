// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ICosmicNoiseStrategy.h"
#include "ThirdParty/FastNoiseLite.h"

/**
 * "Earth-like" procedural noise generation strategy.
 *
 * Combines multiple noise layers (continents, mountains, hills, rivers and detail)
 * along with biome parameters (humidity and temperature) to generate planetary terrain.
 */
class COSMICARCHITECTNOISE_API FCosmicEarthLikeNoiseStrategy : public ICosmicNoiseStrategy
{
public:

    /** Base seed used to initialize all noise systems */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    int32 Seed;

    /** Global terrain height normalization factor */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    float HeightNormalizationScale = 1.0f;

    /** Main noise layer defining continents and landmasses */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseDataLayer ContinentalLayer;

    /** Noise layer used for mountain generation */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseDataLayer MountainLayer;

    /** Noise layer used for hill generation */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseDataLayer HillLayer;

    /** Fine detail terrain noise layer */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseDataLayer DetailLayer;

    /** Noise layer used for river generation */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseDataLayer RiverLayer;

    /** Global biome parameters (humidity, temperature, etc.) */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseBiomeParameters BiomeParameters;

    /**
     * Initializes the noise strategy with all required parameters.
     */
    void Initialize(
        int32 InSeed,
        float InHeightNormalizationScale,
        FCosmicNoiseBiomeParameters InBiomeParameters,
        FCosmicNoiseDataLayer InContinental,
        FCosmicNoiseDataLayer InMountain,
        FCosmicNoiseDataLayer InHill,
        FCosmicNoiseDataLayer InDetail,
        FCosmicNoiseDataLayer InRiver);

    /**
     * Evaluates a point in noise space and generates height and biome color.
     */
    void EvaluatePoint(const FVector& NoiseDir, float& OutHeight, FLinearColor& OutColor) const override;

protected:

    /** Base continent noise */
    FastNoiseLite ContinentalNoise;

    /** Mountain noise (ridged noise) */
    FastNoiseLite MountainNoise;

    /** Hill noise */
    FastNoiseLite HillNoise;

    /** Fine detail noise */
    FastNoiseLite DetailNoise;

    /** River noise based on cellular noise */
    FastNoiseLite RiverNoise;

    /** Humidity noise for biomes */
    FastNoiseLite HumidityNoise;

    /** Temperature noise for biomes */
    FastNoiseLite TempNoise;
};