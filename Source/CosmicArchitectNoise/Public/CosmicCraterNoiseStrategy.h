// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "ICosmicNoiseStrategy.h"
#include "CosmicNoiseTypes.h"
#include "ThirdParty/FastNoiseLite.h"

/**
 * Procedural noise strategy for high-fidelity planetary and lunar impact cratering.
 *
 * Implements:
 * - Sparse Voronoi distribution (eliminates golf-ball honeycomb artifacts).
 * - Central rebound peaks (elastic rebound for complex craters).
 * - Raised rims and radial ejecta blankets.
 * - Non-circular rim domain distortion.
 * - Interior talus rubble and rock breakup.
 * - Planetary vertex color encoding (Normalized Altitude, Fresh Ejecta Rays, Basaltic Maria vs Highlands).
 */
class COSMICARCHITECTNOISE_API FCosmicCraterNoiseStrategy : public ICosmicNoiseStrategy
{
public:
    virtual ~FCosmicCraterNoiseStrategy() override = default;

    int32 Seed = 1337;
    FCosmicNoiseLayer LayerParameters;
    FCosmicNoiseCraterParameters CraterParameters;
    float HeightNormalizationScale = 1.0f;

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
    /** Macro planetary crust topography noise */
    FastNoiseLite BaseNoise;

    /** Cellular distance generator for crater bowl & rim geometry */
    FastNoiseLite CraterDistNoise;

    /** Cellular cell-value hash generator for sparse crater distribution */
    FastNoiseLite CraterCellNoise;

    /** Simplex noise for non-circular crater rim domain distortion */
    FastNoiseLite CraterDistortNoise;

    /** High-frequency micro-breakup roughness on crater walls */
    FastNoiseLite CraterBreakupNoise;

    /** Precomputed normalization extrema */
    float CachedMinHeight = -500.0f;
    float CachedMaxHeight = 500.0f;
};