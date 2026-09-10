// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ICosmicNoiseStrategy.h"
#include "CosmicNoiseTypes.h"
#include "ThirdParty/FastNoiseLite.h"

/**
 * Advanced procedural noise strategy based on Multi-Noise architecture:
 * - Continentalness: Large-scale tectonic crust, abyssal ocean trenches, continental shelves, inland shields.
 * - Erosion: Weathering age that modulates mountain relief and landscape flatness.
 * - Peaks & Valleys: High-relief alpine ridges, knife-edge peaks, canyons, and glacial valleys.
 * - Domain Warping: Inigo Quilez coordinate perturbation preventing synthetic patterns and yielding organic geological meanders.
 * - Planetary Climatology:
 *   * Latitudinal solar gradient (polar ice caps to equatorial heat).
 *   * Adiabatic lapse rate (altitudinal cooling).
 *   * Orographic precipitation & Rain shadow effect (windward moisture condensation vs leeward rain-shadow deserts).
 */
class COSMICARCHITECTNOISE_API FCosmicMultiNoiseStrategy : public ICosmicNoiseStrategy
{
public:
    virtual ~FCosmicMultiNoiseStrategy() override = default;

    /** Base seed used to initialize all procedural noise systems */
    int32 Seed = 1337;

    /** Global terrain height normalization factor */
    float HeightNormalizationScale = 1.0f;

    /** Continentalness noise layer (ocean basins, shelves, continental landmasses) */
    FCosmicNoiseDataLayer ContinentalLayer;

    /** Erosion noise layer (geological age, weathering, relief damping) */
    FCosmicNoiseDataLayer ErosionLayer;

    /** Peaks & Valleys noise layer (alpine ridges, crests, gorges, valleys) */
    FCosmicNoiseDataLayer PeaksValleysLayer;

    /** Micro-detail noise layer (rock faces, surface gravel, boulders) */
    FCosmicNoiseDataLayer DetailLayer;

    /** Specialized Multi-Noise parameters */
    FCosmicMultiNoiseParameters MultiNoiseParams;

    /** Domain warping parameters (Inigo Quilez coordinate distortion) */
    FCosmicNoiseDomainWarpParameters DomainWarpParams;

    /** Global planetary climate parameters (latitudinal gradient, lapse rate, humidity contrast) */
    FCosmicNoiseBiomeParameters BiomeParameters;

    /** Orographic precipitation and rain shadow parameters */
    FCosmicOrographicParameters OrographicParams;

    /**
     * Initializes the strategy and sets up all internal FastNoiseLite generators.
     */
    void Initialize(
        int32 InSeed,
        float InHeightNormalizationScale,
        const FCosmicNoiseDataLayer& InContinental,
        const FCosmicNoiseDataLayer& InErosion,
        const FCosmicNoiseDataLayer& InPeaksValleys,
        const FCosmicNoiseDataLayer& InDetail,
        const FCosmicMultiNoiseParameters& InMultiNoiseParams,
        const FCosmicNoiseDomainWarpParameters& InDomainWarpParams,
        const FCosmicNoiseBiomeParameters& InBiomeParams,
        const FCosmicOrographicParameters& InOrographicParams
    );

    /**
     * Evaluates terrain height and calculates vertex color encoding biome parameters:
     * - R: AltitudeNormalized [0, 1]
     * - G: VisualTemp [0, 1]
     * - B: Humidity [0, 1]
     * - A: 1.0f
     */
    virtual void EvaluatePoint(const FVector& NoiseDir, float& OutHeight, FLinearColor& OutColor) const override;

    /**
     * Fast elevation evaluation without climate/color calculation.
     * Used for efficient upwind slope and mountain barrier sampling.
     */
    float EvaluateHeightOnly(const FVector& NoiseDir) const;

protected:
    /** Computes the local wind tangent vector on the sphere considering atmospheric circulation cells */
    FVector CalculateWindTangent(const FVector& PointOnSphere) const;

    /** Base continent and ocean basin generator */
    FastNoiseLite ContinentalNoise;

    /** Geological erosion and weathering generator */
    FastNoiseLite ErosionNoise;

    /** Alpine peaks and valley generator (ridged fractal) */
    FastNoiseLite PeaksValleysNoise;

    /** High-frequency rock and terrain detail generator */
    FastNoiseLite DetailNoise;

    /** Domain warp coordinate distorter (Inigo Quilez) */
    FastNoiseLite DomainWarpNoise;

    /** Humidity / moisture distribution generator */
    FastNoiseLite HumidityNoise;

    /** Thermal variation generator */
    FastNoiseLite TempNoise;
};
