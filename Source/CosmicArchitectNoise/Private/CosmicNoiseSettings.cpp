// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicNoiseSettings.h"
#include "CosmicNoiseTypes.h"

UCosmicNoiseSettings::UCosmicNoiseSettings()
{
    /*Seed = 1337;
    MaxMountainHeight = 3000.f;
    Mountainous = 0.6f;
    Roughness = 0.4f;
    Detail = 0.7f;
    Smoothness = 0.5f;

    UpdateAdvancedFromSimple();*/
}

void UCosmicNoiseSettings::UpdateAdvancedFromSimple()
{
    UpdateNoiseFromSimple();
    UpdateBiomesFromSimple();
}

void UCosmicNoiseSettings::UpdateBiomesFromSimple()
{
    switch (Params.BiomeType)
    {
    case ECosmicBiomeType::TemperateForest:
        // Bosque templado - húmedo moderado, temperatura moderada
        Params.TemperatureFrequency = 0.004f;
        Params.HumidityFrequency = 0.02f;
        Params.HumidityOctaves = 5;
        Params.LatitudeEffect = 1.0f;
        Params.AltitudeTemperaturePenalty = 0.5f;
        Params.HumidityContrast = 1.3f;
        Params.HumidityOffset = 0.1f;
        break;

    case ECosmicBiomeType::Rainforest:
        // Selva - muy húmedo, cálido
        Params.TemperatureFrequency = 0.003f;
        Params.HumidityFrequency = 0.025f;
        Params.HumidityOctaves = 6;
        Params.LatitudeEffect = 0.8f;
        Params.AltitudeTemperaturePenalty = 0.3f;
        Params.HumidityContrast = 1.2f;
        Params.HumidityOffset = 0.2f;
        break;

    case ECosmicBiomeType::Desert:
        // Desierto - seco, cálido
        Params.TemperatureFrequency = 0.008f;
        Params.HumidityFrequency = 0.01f;
        Params.HumidityOctaves = 3;
        Params.LatitudeEffect = 1.2f;
        Params.AltitudeTemperaturePenalty = 0.7f;
        Params.HumidityContrast = 2.0f;
        Params.HumidityOffset = -0.3f;
        break;

    case ECosmicBiomeType::Tundra:
        // Tundra - frío extremo, seco
        Params.TemperatureFrequency = 0.001f;
        Params.HumidityFrequency = 0.015f;
        Params.HumidityOctaves = 4;
        Params.LatitudeEffect = 1.5f;
        Params.AltitudeTemperaturePenalty = 0.9f;
        Params.HumidityContrast = 1.5f;
        Params.HumidityOffset = -0.2f;
        break;

    case ECosmicBiomeType::Taiga:
        // Taiga - frío moderado, húmedo moderado
        Params.TemperatureFrequency = 0.002f;
        Params.HumidityFrequency = 0.018f;
        Params.HumidityOctaves = 5;
        Params.LatitudeEffect = 1.3f;
        Params.AltitudeTemperaturePenalty = 0.8f;
        Params.HumidityContrast = 1.4f;
        Params.HumidityOffset = 0.0f;
        break;

    case ECosmicBiomeType::Savannah:
        // Sabana - cálido, seco moderado
        Params.TemperatureFrequency = 0.006f;
        Params.HumidityFrequency = 0.012f;
        Params.HumidityOctaves = 4;
        Params.LatitudeEffect = 1.1f;
        Params.AltitudeTemperaturePenalty = 0.6f;
        Params.HumidityContrast = 1.8f;
        Params.HumidityOffset = -0.1f;
        break;

    case ECosmicBiomeType::Grassland:
        // Pradera - templado, moderado
        Params.TemperatureFrequency = 0.004f;
        Params.HumidityFrequency = 0.015f;
        Params.HumidityOctaves = 4;
        Params.LatitudeEffect = 1.0f;
        Params.AltitudeTemperaturePenalty = 0.5f;
        Params.HumidityContrast = 1.2f;
        Params.HumidityOffset = 0.0f;
        break;

    case ECosmicBiomeType::Swamp:
        // Pantano - húmedo, cálido
        Params.TemperatureFrequency = 0.005f;
        Params.HumidityFrequency = 0.03f;
        Params.HumidityOctaves = 6;
        Params.LatitudeEffect = 0.9f;
        Params.AltitudeTemperaturePenalty = 0.4f;
        Params.HumidityContrast = 1.1f;
        Params.HumidityOffset = 0.3f;
        break;

    case ECosmicBiomeType::Volcanic:
        // Volcánico - caliente extremo
        Params.TemperatureFrequency = 0.02f;
        Params.HumidityFrequency = 0.008f;
        Params.HumidityOctaves = 3;
        Params.LatitudeEffect = 0.5f;
        Params.AltitudeTemperaturePenalty = 0.1f;
        Params.HumidityContrast = 2.5f;
        Params.HumidityOffset = -0.4f;
        break;

    case ECosmicBiomeType::Alien:
        // Alienígena - caótico
        Params.TemperatureFrequency = 0.015f;
        Params.HumidityFrequency = 0.04f;
        Params.HumidityOctaves = 7;
        Params.LatitudeEffect = 0.3f;
        Params.AltitudeTemperaturePenalty = 0.2f;
        Params.HumidityContrast = 3.0f;
        Params.HumidityOffset = 0.0f;
        break;

    case ECosmicBiomeType::Ocean:
        // Océano - húmedo siempre
        Params.TemperatureFrequency = 0.004f;
        Params.HumidityFrequency = 0.025f;
        Params.HumidityOctaves = 5;
        Params.LatitudeEffect = 1.0f;
        Params.AltitudeTemperaturePenalty = 0.0f;
        Params.HumidityContrast = 1.0f;
        Params.HumidityOffset = 0.5f;
        break;

    case ECosmicBiomeType::Ice:
        // Hielo - frío extremo
        Params.TemperatureFrequency = 0.0005f;
        Params.HumidityFrequency = 0.01f;
        Params.HumidityOctaves = 4;
        Params.LatitudeEffect = 1.8f;
        Params.AltitudeTemperaturePenalty = 0.95f;
        Params.HumidityContrast = 1.2f;
        Params.HumidityOffset = 0.1f;
        break;

    case ECosmicBiomeType::Cratered:
    default:
        // Lunar - temperaturas extremas
        Params.TemperatureFrequency = 0.03f;
        Params.HumidityFrequency = 0.001f;
        Params.HumidityOctaves = 1;
        Params.LatitudeEffect = 2.0f;
        Params.AltitudeTemperaturePenalty = 0.5f;
        Params.HumidityContrast = 1.0f;
        Params.HumidityOffset = -0.5f;
        break;
    }
}

void UCosmicNoiseSettings::UpdateNoiseFromSimple()
{

    Params.Biomes.Empty();
    
    FCosmicBiomeData SimpleBiome;
    SimpleBiome.BiomeName = "Simple Global Biome";

    SimpleBiome.TargetTemperature = 0.5f;
    SimpleBiome.TargetHumidity = 0.5f;

    FCosmicNoiseTypes Layer;
    Layer.NoiseType = ECosmicNoiseType::Simplex;
    Layer.FractalType = ECosmicFractalType::FBM;
    Layer.Frequency = FMath::Lerp(0.01f, 10.f, 1.f - Params.Smoothness);
    Layer.Octaves = FMath::RoundToInt(FMath::Lerp(2.f, 12.f, Params.Detail));
    Layer.Lacunarity = FMath::Lerp(1.8f, 2.3f, Params.Roughness);
    Layer.Persistence = FMath::Lerp(0.4f, 0.65f, Params.Mountainous);
    Layer.Amplitude = Params.MaxMountainHeight;

    SimpleBiome.NoiseLayers.Add(Layer);

    Params.Biomes.Add(SimpleBiome);
    Params.Biomes.Empty();

    // Warp proporcional a la altura máxima
    Params.DomainWarpStrength = Params.MaxMountainHeight * FMath::Lerp(0.02f, 0.15f, Params.Roughness);

    // Frecuencia alineada con la base
    Params.DomainWarpFrequency = FMath::Lerp(0.001f, 0.004f, Params.Roughness);
}

#if WITH_EDITOR
void UCosmicNoiseSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent); // Siempre llama al Super primero o al inicio

    // Obtenemos la propiedad que cambió y, si es parte de un struct, su propiedad "padre"
    FProperty* Property = PropertyChangedEvent.Property;
    FProperty* MemberProperty = PropertyChangedEvent.MemberProperty;

    if (!Property || !MemberProperty) return;

    FName PropertyName = Property->GetFName();
    FName MemberPropertyName = MemberProperty->GetFName();

    // Si no estamos en modo avanzado, sincronizamos
    if (!Params.bUseAdvancedSettings)
    {
        // Si el cambio ocurrió DENTRO de "Params"
        if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UCosmicNoiseSettings, Params))
        {
            // Comprobamos qué campo específico del struct cambió
            if (PropertyName == "BiomeType")
            {
                UpdateBiomesFromSimple();
            }
            else if (PropertyName == "Seed" ||
                PropertyName == "MaxMountainHeight" ||
                PropertyName == "Mountainous" ||
                PropertyName == "Roughness" ||
                PropertyName == "Detail" ||
                PropertyName == "Smoothness")
            {
                UpdateNoiseFromSimple();
            }
        }
    }
}
#endif
