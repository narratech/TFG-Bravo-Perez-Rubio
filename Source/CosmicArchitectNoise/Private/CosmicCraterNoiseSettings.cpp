

#include "CosmicCraterNoiseSettings.h"
#include "CosmicCraterNoiseStrategy.h"

TSharedPtr<ICosmicNoiseStrategy> UCosmicCraterNoiseSettings::CreateStrategy() const
{
    auto Strategy = MakeShared<FCosmicCraterNoiseStrategy>();

    Strategy->Initialize(Seed, LayerParameters, BiomeParameters, CraterParameters);

    return Strategy;
}
 
