// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CosmicNoiseClass.h"
#include "CosmicCraterNoiseSettings.generated.h"

/**
 * Configuration used to generate a crater-based noise strategy.
 *
 * This class encapsulates all parameters required to initialize
 * an instance of FCosmicCraterNoiseStrategy.
 */
UCLASS()
class COSMICARCHITECTNOISE_API UCosmicCraterNoiseSettings : public UCosmicNoiseClass
{
    GENERATED_BODY()

public:

    /**
     * Seed used for procedural noise generation.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    int32 Seed;

    /**
     * General parameters of the noise layers.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseLayer LayerParameters;

    /**
     * Parameters related to biome generation.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseBiomeParameters BiomeParameters;

    /**
     * Specific parameters for crater generation.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseCraterParameters CraterParameters;

    /**
     * Creates and initializes the crater noise strategy
     * using parameters configured in this class.
     *
     * @return Fully initialized noise strategy.
     */
    virtual TSharedPtr<ICosmicNoiseStrategy> CreateStrategy() const override;
};