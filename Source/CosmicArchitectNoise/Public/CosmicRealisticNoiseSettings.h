// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CosmicNoiseClass.h"
#include "CosmicRealisticNoiseSettings.generated.h"

/**
 * 
 */
UCLASS()
class COSMICARCHITECTNOISE_API UCosmicRealisticNoiseSettings : public UCosmicNoiseClass
{
	GENERATED_BODY()
	
    /**
     * Seed used for procedural terrain generation.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    int32 Seed;

    /**
     * Scale used to normalize final terrain height.
     *
     *
     * Lower values compress heights.
     * Higher values preserve greater vertical variation.
     */
    UPROPERTY(
        EditAnywhere,
        Category = "Noise Settings",
        meta = (ClampMin = "0.0", ClampMax = "1.0")
    )
    float HeightNormalizationScale = 1.0f;

    /**
     * Main layer used to generate continents.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseDataLayer ContinentalLayer;

    /**
     * Layer used to generate mountain ranges.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseDataLayer MountainLayer;

    /**
     * Layer used to generate hills and smooth variations.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseDataLayer HillLayer;

    /**
     * Layer used to add fine detail to terrain.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseDataLayer DetailLayer;

    /**
     * Layer used for procedural river generation.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseDataLayer RiverLayer;

    /**
     * Layer used for procedural river generation.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseBiomeParameters BiomeParameters;

    /**
     * Creates and initializes the realistic noise strategy
     * using the current configuration.
     *
     * @return Fully initialized noise strategy.
     */
    virtual TSharedPtr<ICosmicNoiseStrategy> CreateStrategy() const override;
};
