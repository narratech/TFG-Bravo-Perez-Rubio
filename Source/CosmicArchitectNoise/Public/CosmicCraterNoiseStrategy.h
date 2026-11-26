// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "ICosmicNoiseStrategy.h"
#include "CosmicNoiseTypes.h"
#include "CosmicCraterNoiseSettings.h"
#include "ThirdParty/FastNoiseLite.h"

/**
 * Procedural noise strategy for high-fidelity planetary and lunar impact cratering.
 *
 * Implements:
 * - Domain-warped primordial crust with macro basaltic maria basins vs rugged highland plateaus.
 * - Hybrid ridged multifractal mountain ranges (montes) and maria wrinkle ridges (dorsa).
 * - Musgrave heterogeneous ValueCubic micro-detail (smooth dusty maria, rough fractured highlands).
 * - Sparse Voronoi multi-scale craters with normalized cell hashing, frequency-independent distortion,
 *   age weathering, maria lava flooding, floor excavation flattening, slumped inner wall terraces,
 *   complex central rebound peaks, and broken ejecta blankets.
 * - Full planetary climatology vertex color encoding (R: Altitude, G: Visual Temperature, B: Humidity/Biome).
 */
class COSMICARCHITECTNOISE_API FCosmicCraterNoiseStrategy : public ICosmicNoiseStrategy
{
public:
    virtual ~FCosmicCraterNoiseStrategy() override = default;

    int32 Seed = 1337;
    FCosmicNoiseLayer LayerParameters;
    FCosmicNoiseCraterParameters CraterParameters;
    float HeightNormalizationScale = 1.0f;

    FCosmicNoiseDataLayer RidgeLayer;
    FCosmicNoiseDataLayer DetailLayer;
    FCosmicCraterTerrainParameters TerrainParameters;
    FCosmicNoiseDomainWarpParameters DomainWarpParams;
    FCosmicNoiseBiomeParameters BiomeParameters;

    /** Primary initialization from settings asset. */
    void Initialize(const UCosmicCraterNoiseSettings& InSettings);

    /** Legacy initialization overloads preserved for backward compatibility. */
    void Initialize(
        int32 InSeed,
        const FCosmicNoiseLayer& InLayerParameters,
        const FCosmicNoiseCraterParameters& InCraterParameters,
        float InHeightNormalizationScale = 1.0f
    );

    void Initialize(
        int32 InSeed,
        const FCosmicNoiseLayer& InLayerParameters,
        const FCosmicNoiseBiomeParameters& InBiomeParameters,
        const FCosmicNoiseCraterParameters& InCraterParameters
    );

    virtual void EvaluatePoint(
        const FVector& NoiseDir,
        float& OutHeight,
        FLinearColor& OutColor
    ) const override;

protected:
    /** Configures all FastNoiseLite instances and caches elevation normalization bounds. */
    void SetupGenerators();

    /** Macro crustal domain warping (Inigo Quilez progressive warp) */
    FastNoiseLite DomainWarpNoise;

    /** Continent-scale basaltic maria basins vs primordial highlands mask */
    FastNoiseLite MariaBasinNoise;

    /** Macro planetary crust topography noise */
    FastNoiseLite BaseNoise;

    /** Hybrid ridged multifractal for highland mountain chains (montes) and wrinkle ridges (dorsa) */
    FastNoiseLite RidgeNoise;

    /** High-frequency ValueCubic micro-detail (regolith, rock facets) */
    FastNoiseLite DetailNoise;

    /** Cellular distance generator for crater bowl & rim geometry */
    FastNoiseLite CraterDistNoise;

    /** Cellular cell-value hash generator for sparse crater distribution */
    FastNoiseLite CraterCellNoise;

    /** Simplex noise for non-circular crater rim domain distortion */
    FastNoiseLite CraterDistortNoise;

    /** High-frequency micro-breakup roughness on crater walls and ejecta blankets */
    FastNoiseLite CraterBreakupNoise;

    /** Planetary climate noise generators (Temperature & Humidity) */
    FastNoiseLite TempNoise;
    FastNoiseLite HumidityNoise;

    /** Precomputed statistical normalization extrema */
    float CachedMinHeight = -500.0f;
    float CachedMaxHeight = 500.0f;
};