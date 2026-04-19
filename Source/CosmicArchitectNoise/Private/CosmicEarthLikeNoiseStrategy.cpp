// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicEarthLikeNoiseStrategy.h"

void FCosmicEarthLikeNoiseStrategy::Initialize(int32 InSeed, FCosmicNoiseBiomeParameters InBiomeParameters)
{
    Seed = InSeed;
    BiomeParameters = InBiomeParameters;

    HumidityNoise.SetSeed(Seed + 128);
    HumidityNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    HumidityNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    HumidityNoise.SetFrequency(BiomeParameters.HumidityFrequency);
    HumidityNoise.SetFractalOctaves(BiomeParameters.HumidityOctaves);

    TempNoise.SetSeed(Seed + 256);
    TempNoise.SetFrequency(BiomeParameters.TemperatureFrequency);
}

void FCosmicEarthLikeNoiseStrategy::EvaluatePoint(const FVector& NoiseDir, float& OutHeight, FLinearColor& OutColor) const
{

}
