// Copyright 1998 - 2026 Epic Games, Inc. All Rights Reserved.


#include "CosmicDefaultNoiseSettings.h"
#include "CosmicDefaultNoiseStrategy.h"

TSharedPtr<ICosmicNoiseStrategy> UCosmicDefaultNoiseSettings::CreateStrategy() const
{
    auto Strategy = MakeShared<FCosmicDefaultNoiseStrategy>();

    Strategy->Initialize(Seed, LayerParameters, BiomeParameters);

    return Strategy;
}
 