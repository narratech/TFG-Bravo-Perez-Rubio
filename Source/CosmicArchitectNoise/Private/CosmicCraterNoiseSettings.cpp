// Copyright 1998 - 2026 Epic Games, Inc. All Rights Reserved.


#include "CosmicCraterNoiseSettings.h"
#include "CosmicCraterNoiseStrategy.h"

TSharedPtr<ICosmicNoiseStrategy> UCosmicCraterNoiseSettings::CreateStrategy() const
{
    auto Strategy = MakeShared<FCosmicCraterNoiseStrategy>();

    Strategy->Initialize(Seed, LayerParameters, BiomeParameters, CraterParameters);

    return Strategy;
}
 