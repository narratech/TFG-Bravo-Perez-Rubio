// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CosmicNoiseClass.h"
#include "CosmicNoiseTypes.h"
#include "CosmicCraterNoiseSettings.generated.h"

/**
 * Geological terrain parameters controlling non-crater planetary relief,
 * basaltic maria basins, ridged highland mountains, and crater weathering.
 */
USTRUCT(BlueprintType)
struct COSMICARCHITECTNOISE_API FCosmicCraterTerrainParameters
{
    GENERATED_BODY()

    /** Exponent power applied to ridged highland peaks and dorsa (higher = sharper alpine/tectonic ridges). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crater Terrain", meta = (ClampMin = "1.0", ClampMax = "4.0"))
    float RidgePeakSharpness = 1.65f;

    /** Weighted strength for FastNoiseLite ridged multifractal inter-octave feedback. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crater Terrain", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float RidgeWeightedStrength = 0.80f;

    /** Depth scale for depressed basaltic maria lowlands relative to base amplitude. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crater Terrain", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float MariaBasinDepthScale = 0.55f;

    /** Musgrave lowland smoothing in maria basins (0.0 = glass-smooth lava plains, 1.0 = same roughness as highlands). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crater Terrain", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MariaRoughnessFactor = 0.22f;

    /** How strongly lava flooding in deep maria basins softens and fills older impact craters. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crater Terrain", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MariaCraterFlooding = 0.45f;

    /** How strongly large crater bowls excavate and flatten pre-existing terrain underneath their floors. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crater Terrain", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CraterFloorFlattening = 0.65f;

    /** Age/weathering variation across craters (0.0 = all crisp young impacts, 1.0 = mix of crisp young and eroded ancient craters). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crater Terrain", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CraterAgeVariation = 0.45f;

    /** Subtle slumped terrace stepping on the inner walls of large complex craters. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crater Terrain", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float InnerWallTerracing = 0.25f;

    /** Radial/angular noise breakup on outer ejecta blankets so rims do not form uniform rings. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crater Terrain", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float EjectaRayBreakup = 0.55f;

    /** Subtle influence of crater ejecta and basins on temperature/humidity vertex colors (kept low so biomes span the whole planet). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crater Terrain", meta = (ClampMin = "0.0", ClampMax = "0.5"))
    float CraterBiomeInfluence = 0.15f;
};

/**
 * Configuration for the realistic planetary/lunar crater noise strategy.
 *
 * Combines:
 * - Domain-warped primordial crust with macro basaltic maria basins vs rugged highland plateaus.
 * - Hybrid ridged multifractal mountain ranges (montes) and maria wrinkle ridges (dorsa).
 * - Musgrave heterogeneous micro-detail (smooth dusty maria, rough fractured highlands).
 * - Multi-scale sparse Voronoi impact craters with age weathering, floor excavation, slumped walls, central peaks, and broken ejecta.
 * - Full planetary climatology vertex color encoding (R: Normalized Altitude, G: Visual Temperature, B: Humidity/Biome).
 */
UCLASS(BlueprintType)
class COSMICARCHITECTNOISE_API UCosmicCraterNoiseSettings : public UCosmicNoiseClass
{
    GENERATED_BODY()

public:
    UCosmicCraterNoiseSettings();

    /** Seed used for procedural generation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Settings")
    int32 Seed = 1337;

    /** General parameters of the macro planetary base terrain layer */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Settings")
    FCosmicNoiseLayer LayerParameters;

    /** Specific parameters for multi-scale crater generation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Settings")
    FCosmicNoiseCraterParameters CraterParameters;

    /** Height normalization scale factor */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Settings", meta = (ClampMin = "0.01", ClampMax = "5.0"))
    float HeightNormalizationScale = 1.0f;

    /** Hybrid ridged multifractal layer for highland mountain chains (montes) and wrinkle ridges (dorsa). Amplitude is relative to LayerParameters.Amplitude. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Layers")
    FCosmicNoiseDataLayer RidgeLayer;

    /** High-frequency ValueCubic micro-detail layer (regolith, rocks, facets). Amplitude is relative to LayerParameters.Amplitude. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Layers")
    FCosmicNoiseDataLayer DetailLayer;

    /** Geological terrain shaping for maria basins, ridged highlands, and crater weathering */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geological Features")
    FCosmicCraterTerrainParameters TerrainParameters;

    /** Domain warping parameters (Inigo Quilez coordinate perturbation for organic crustal geology) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Domain Warping")
    FCosmicNoiseDomainWarpParameters DomainWarpParams;

    /** Planetary climate and biome parameters (drives vertex color G = Temperature, B = Humidity across the entire globe) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Settings")
    FCosmicNoiseBiomeParameters BiomeParameters;

    /**
     * Creates and initializes the crater noise strategy.
     *
     * @return Fully initialized noise strategy.
     */
    virtual TSharedPtr<ICosmicNoiseStrategy> CreateStrategy() const override;
};