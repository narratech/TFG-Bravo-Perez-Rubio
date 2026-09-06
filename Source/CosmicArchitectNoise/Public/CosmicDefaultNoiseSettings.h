// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CosmicNoiseClass.h"
#include "CosmicDefaultNoiseSettings.generated.h"

/**
 * Configuration used to create a standard procedural noise strategy.
 *
 * This class encapsulates parameters required to initialize
 * an instance of FCosmicDefaultNoiseStrategy.
 */
UCLASS()
class COSMICARCHITECTNOISE_API UCosmicDefaultNoiseSettings : public UCosmicNoiseClass
{
    GENERATED_BODY()

public:

    /**
     * Seed used for procedural noise generation.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    int32 Seed;

    /**
     * General parameters of the main noise layer.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseLayer LayerParameters;

    /**
     * Parameters used for biome generation and blending.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseBiomeParameters BiomeParameters;

    /**
     * Creates and initializes the default noise strategy
     * using the current configuration.
     *
     * @return Fully initialized noise strategy.
     */
    virtual TSharedPtr<ICosmicNoiseStrategy> CreateStrategy() const override;
};