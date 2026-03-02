// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicSettingsResolver.h"
#include "CosmicNoiseSettings.h"


void CosmicSettingsResolver::Resolve(const UCosmicNoiseSettings* Settings, TArray<FCosmicNoiseTypes>& OutLayers, bool& bOutUseWarp, float& OutWarpStrength, float& OutWarpFrequency)
{
    if (!Settings)
    {
        OutLayers.Empty();
        bOutUseWarp = false;
        OutWarpStrength = 0.0f;
        OutWarpFrequency = 0.0f;
        return;
    }

    if (Settings->bUseAdvancedSettings)
    {
        OutLayers = Settings->NoiseLayers;
        bOutUseWarp = Settings->bUseDomainWarp;
        OutWarpStrength = Settings->DomainWarpStrength;
        OutWarpFrequency = Settings->DomainWarpFrequency;
        return;
    }

    // SIMPLE MODE traducir conceptos a capas técnicas
    bOutUseWarp = false;
    OutWarpStrength = 0.0f;
    OutWarpFrequency = 0.0f;

    OutLayers.Reset();

    // Base terrain (capa base de baja frecuencia)
    FCosmicNoiseTypes Base;
    Base.NoiseType = ECosmicNoiseType::Perlin;
    Base.FractalType = ECosmicFractalType::FBM;
    Base.Frequency = FMath::Lerp(0.0001f, 0.002f, Settings->Smoothness);
    Base.Octaves = 3;
    Base.Lacunarity = 2.0f;
    Base.Persistence = 0.5f;
    Base.Amplitude = Settings->MaxMountainHeight * 0.4f;
    OutLayers.Add(Base);

    // Mountains (capa de montañas con ruido ridged)
    FCosmicNoiseTypes Mountains;
    Mountains.NoiseType = ECosmicNoiseType::Ridged;
    Mountains.FractalType = ECosmicFractalType::Ridged;
    Mountains.Frequency = 0.003f;
    Mountains.Octaves = 4;
    Mountains.Lacunarity = 2.0f;
    Mountains.Persistence = 0.5f;
    Mountains.Amplitude = Settings->Mountainous * Settings->MaxMountainHeight;
    OutLayers.Add(Mountains);

    // Detail (capa de detalle fino)
    FCosmicNoiseTypes Detail;
    Detail.NoiseType = ECosmicNoiseType::Simplex;
    Detail.FractalType = ECosmicFractalType::FBM;
    Detail.Frequency = 0.02f;
    Detail.Octaves = 2;
    Detail.Lacunarity = 2.0f;
    Detail.Persistence = 0.5f;
    Detail.Amplitude = Settings->Detail * 200.0f;
    OutLayers.Add(Detail);
}
