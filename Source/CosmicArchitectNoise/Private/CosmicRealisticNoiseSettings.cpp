// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicRealisticNoiseSettings.h"
#include "CosmicRealisticNoiseStrategy.h"

TSharedPtr<ICosmicNoiseStrategy> UCosmicRealisticNoiseSettings::CreateStrategy() const
{
    auto Strategy = MakeShared<FCosmicRealisticNoiseStrategy>();

    Strategy->Initialize(Seed, HeightNormalizationScale, BiomeParameters,
        ContinentalLayer, MountainLayer, HillLayer, DetailLayer, RiverLayer);

    return Strategy;
}
