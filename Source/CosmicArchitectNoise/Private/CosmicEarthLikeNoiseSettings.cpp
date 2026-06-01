// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicEarthLikeNoiseSettings.h"
#include "CosmicEarthLikeNoiseStrategy.h"

TSharedPtr<ICosmicNoiseStrategy> UCosmicEarthLikeNoiseSettings::CreateStrategy() const
{

    auto Strategy = MakeShared<FCosmicEarthLikeNoiseStrategy>();

    Strategy->Initialize(Seed, HeightNormalizationScale, BiomeParameters,
        ContinentalLayer, MountainLayer, HillLayer, DetailLayer, RiverLayer);

    return Strategy;
}
 