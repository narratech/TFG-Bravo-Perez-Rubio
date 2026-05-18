// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicEarthLikeNoiseSettings.h"
#include "CosmicEarthLikeNoiseStrategy.h"

TSharedPtr<ICosmicNoiseStrategy> UCosmicEarthLikeNoiseSettings::CreateStrategy() const
{

    auto Strategy = MakeShared<FCosmicEarthLikeNoiseStrategy>();

    Strategy->Initialize(Seed, HeightNormalizationScale, BiomeParameters,
        ContinentalLayer, MountainLayer, HillLayer, DetailLayer, RiverLayer);

    return Strategy;
}
