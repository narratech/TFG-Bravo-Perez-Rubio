// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.


#include "CosmicRealisticNoiseSettings.h"
#include "CosmicRealisticNoiseStrategy.h"

TSharedPtr<ICosmicNoiseStrategy> UCosmicRealisticNoiseSettings::CreateStrategy() const
{
    auto Strategy = MakeShared<FCosmicRealisticNoiseStrategy>();

    Strategy->Initialize(Seed, HeightNormalizationScale, BiomeParameters,
        ContinentalLayer, MountainLayer, HillLayer, DetailLayer, RiverLayer);

    return Strategy;
}
