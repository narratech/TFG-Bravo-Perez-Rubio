// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicDefaultNoiseSettings.h"
#include "CosmicDefaultNoiseStrategy.h"

TSharedPtr<ICosmicNoiseStrategy> UCosmicDefaultNoiseSettings::CreateStrategy() const
{
    auto Strategy = MakeShared<FCosmicDefaultNoiseStrategy>();

    Strategy->Initialize(Seed, LayerParameters, BiomeParameters);

    return Strategy;
}
 