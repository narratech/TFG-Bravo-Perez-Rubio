// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CosmicNoiseClass.h"
#include "CosmicCraterNoiseSettings.generated.h"

/**
 * Configuration for the realistic planetary/lunar crater noise strategy.
 *
 * Simulates high-fidelity impact craters:
 * - Sparse Poisson-like Voronoi distribution (eliminates golf-ball honeycomb artifacts).
 * - Multi-scale crater hierarchy (giant basins, complex craters with central peaks, micro-impacts).
 * - Central rebound peaks (isostatic elastic rebound).
 * - Raised rims and radial ejecta blankets.
 * - Planetary vertex color encoding (R: Normalized Altitude, G: Fresh Ejecta Rays, B: Maria Basalt vs Highlands).
 */
UCLASS()
class COSMICARCHITECTNOISE_API UCosmicCraterNoiseSettings : public UCosmicNoiseClass
{
    GENERATED_BODY()

public:

    /** Seed used for procedural generation */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    int32 Seed = 1337;

    /** General parameters of the macro planetary base terrain layer */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseLayer LayerParameters;

    /** Specific parameters for multi-scale crater generation */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseCraterParameters CraterParameters;

    /** Height normalization scale factor */
    UPROPERTY(EditAnywhere, Category = "Noise Settings", meta = (ClampMin = "0.01", ClampMax = "5.0"))
    float HeightNormalizationScale = 1.0f;

    /**
     * Creates and initializes the crater noise strategy.
     *
     * @return Fully initialized noise strategy.
     */
    virtual TSharedPtr<ICosmicNoiseStrategy> CreateStrategy() const override;
};