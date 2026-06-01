// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicDefaultNoiseSettings.h"
#include "CosmicDefaultNoiseStrategy.h"

TSharedPtr<ICosmicNoiseStrategy> UCosmicDefaultNoiseSettings::CreateStrategy() const
{
    auto Strategy = MakeShared<FCosmicDefaultNoiseStrategy>();

    Strategy->Initialize(Seed, LayerParameters, BiomeParameters);

    return Strategy;
}
 