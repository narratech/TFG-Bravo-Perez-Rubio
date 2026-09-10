// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CosmicNoiseClass.h"
#include "CosmicNoiseTypes.h"
#include "CosmicMultiNoiseSettings.generated.h"

/**
 * DataAsset configuration for Multi-Noise Planetary Procedural Generation.
 *
 * Implements modern multi-noise terrain architecture:
 * - Continentalness: Large-scale tectonic crust, ocean basins, shelves, and continental shields.
 * - Erosion: Geological age and weathering that modulates relief and sharpness.
 * - Peaks & Valleys: Alpine mountain ridges, knife-edge peaks, canyons, and glacial valleys.
 * - Domain Warping: Inigo Quilez coordinate perturbation for natural sinuous geological structures.
 * - Planetary Climatology:
 *   * Latitudinal polar thermal gradient.
 *   * Adiabatic lapse rate (altitudinal cooling).
 *   * Orographic precipitation (windward slopes) and Rain Shadow effect (leeward deserts).
 */
UCLASS(BlueprintType)
class COSMICARCHITECTNOISE_API UCosmicMultiNoiseSettings : public UCosmicNoiseClass
{
    GENERATED_BODY()

public:
    UCosmicMultiNoiseSettings();

    /** Procedural seed used to initialize all noise generators */
    UPROPERTY(EditAnywhere, Category = "MultiNoise - General", BlueprintReadWrite)
    int32 Seed = 1337;

    /** Scale used to normalize final terrain height for vertex colors */
    UPROPERTY(EditAnywhere, Category = "MultiNoise - General", BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float HeightNormalizationScale = 1.0f;

    /** Continentalness Layer (Ocean basins, continental shelf, and continental shields) */
    UPROPERTY(EditAnywhere, Category = "MultiNoise - Layers", BlueprintReadWrite)
    FCosmicNoiseDataLayer ContinentalLayer;

    /** Erosion Layer (Weathering age, flatness modulation, and relief damping) */
    UPROPERTY(EditAnywhere, Category = "MultiNoise - Layers", BlueprintReadWrite)
    FCosmicNoiseDataLayer ErosionLayer;

    /** Peaks & Valleys Layer (Alpine ridges, peaks, canyons, and valleys) */
    UPROPERTY(EditAnywhere, Category = "MultiNoise - Layers", BlueprintReadWrite)
    FCosmicNoiseDataLayer PeaksValleysLayer;

    /** Micro-Detail Layer (Rock roughness, gravel, high-frequency boulders) */
    UPROPERTY(EditAnywhere, Category = "MultiNoise - Layers", BlueprintReadWrite)
    FCosmicNoiseDataLayer DetailLayer;

    /** Specialized Multi-Noise parameters (sea level, shelf width, mountain sharpness, terracing) */
    UPROPERTY(EditAnywhere, Category = "MultiNoise - Parameters", BlueprintReadWrite)
    FCosmicMultiNoiseParameters MultiNoiseParams;

    /** Domain Warping parameters (Inigo Quilez coordinate distortion) */
    UPROPERTY(EditAnywhere, Category = "MultiNoise - Domain Warping", BlueprintReadWrite)
    FCosmicNoiseDomainWarpParameters DomainWarpParams;

    /** Global planetary climate parameters (latitudinal gradient, lapse rate, humidity contrast) */
    UPROPERTY(EditAnywhere, Category = "MultiNoise - Climate & Biomes", BlueprintReadWrite)
    FCosmicNoiseBiomeParameters BiomeParameters;

    /** Orographic precipitation and rain shadow parameters */
    UPROPERTY(EditAnywhere, Category = "MultiNoise - Climate & Biomes", BlueprintReadWrite)
    FCosmicOrographicParameters OrographicParams;

    /**
     * Creates and initializes the multi-noise strategy using the current configuration.
     *
     * @return Fully initialized noise strategy instance.
     */
    virtual TSharedPtr<ICosmicNoiseStrategy> CreateStrategy() const override;
};
