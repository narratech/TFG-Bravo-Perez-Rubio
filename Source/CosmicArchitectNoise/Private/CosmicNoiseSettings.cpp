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
    switch (BiomeType)
    {
    case ECosmicBiomeType::TemperateForest:
        // Bosque templado - húmedo moderado, temperatura moderada
        TemperatureFrequency = 0.004f;
        HumidityFrequency = 0.02f;
        HumidityOctaves = 5;
        LatitudeEffect = 1.0f;
        AltitudeTemperaturePenalty = 0.5f;
        HumidityContrast = 1.3f;
        HumidityOffset = 0.1f;
        break;

    case ECosmicBiomeType::Rainforest:
        // Selva - muy húmedo, cálido
        TemperatureFrequency = 0.003f;
        HumidityFrequency = 0.025f;
        HumidityOctaves = 6;
        LatitudeEffect = 0.8f;
        AltitudeTemperaturePenalty = 0.3f;
        HumidityContrast = 1.2f;
        HumidityOffset = 0.2f;
        break;

    case ECosmicBiomeType::Desert:
        // Desierto - seco, cálido
        TemperatureFrequency = 0.008f;
        HumidityFrequency = 0.01f;
        HumidityOctaves = 3;
        LatitudeEffect = 1.2f;
        AltitudeTemperaturePenalty = 0.7f;
        HumidityContrast = 2.0f;
        HumidityOffset = -0.3f;
        break;

    case ECosmicBiomeType::Tundra:
        // Tundra - frío extremo, seco
        TemperatureFrequency = 0.001f;
        HumidityFrequency = 0.015f;
        HumidityOctaves = 4;
        LatitudeEffect = 1.5f;
        AltitudeTemperaturePenalty = 0.9f;
        HumidityContrast = 1.5f;
        HumidityOffset = -0.2f;
        break;

    case ECosmicBiomeType::Taiga:
        // Taiga - frío moderado, húmedo moderado
        TemperatureFrequency = 0.002f;
        HumidityFrequency = 0.018f;
        HumidityOctaves = 5;
        LatitudeEffect = 1.3f;
        AltitudeTemperaturePenalty = 0.8f;
        HumidityContrast = 1.4f;
        HumidityOffset = 0.0f;
        break;

    case ECosmicBiomeType::Savannah:
        // Sabana - cálido, seco moderado
        TemperatureFrequency = 0.006f;
        HumidityFrequency = 0.012f;
        HumidityOctaves = 4;
        LatitudeEffect = 1.1f;
        AltitudeTemperaturePenalty = 0.6f;
        HumidityContrast = 1.8f;
        HumidityOffset = -0.1f;
        break;

    case ECosmicBiomeType::Grassland:
        // Pradera - templado, moderado
        TemperatureFrequency = 0.004f;
        HumidityFrequency = 0.015f;
        HumidityOctaves = 4;
        LatitudeEffect = 1.0f;
        AltitudeTemperaturePenalty = 0.5f;
        HumidityContrast = 1.2f;
        HumidityOffset = 0.0f;
        break;

    case ECosmicBiomeType::Swamp:
        // Pantano - húmedo, cálido
        TemperatureFrequency = 0.005f;
        HumidityFrequency = 0.03f;
        HumidityOctaves = 6;
        LatitudeEffect = 0.9f;
        AltitudeTemperaturePenalty = 0.4f;
        HumidityContrast = 1.1f;
        HumidityOffset = 0.3f;
        break;

    case ECosmicBiomeType::Volcanic:
        // Volcánico - caliente extremo
        TemperatureFrequency = 0.02f;
        HumidityFrequency = 0.008f;
        HumidityOctaves = 3;
        LatitudeEffect = 0.5f;
        AltitudeTemperaturePenalty = 0.1f;
        HumidityContrast = 2.5f;
        HumidityOffset = -0.4f;
        break;

    case ECosmicBiomeType::Alien:
        // Alienígena - caótico
        TemperatureFrequency = 0.015f;
        HumidityFrequency = 0.04f;
        HumidityOctaves = 7;
        LatitudeEffect = 0.3f;
        AltitudeTemperaturePenalty = 0.2f;
        HumidityContrast = 3.0f;
        HumidityOffset = 0.0f;
        break;

    case ECosmicBiomeType::Ocean:
        // Océano - húmedo siempre
        TemperatureFrequency = 0.004f;
        HumidityFrequency = 0.025f;
        HumidityOctaves = 5;
        LatitudeEffect = 1.0f;
        AltitudeTemperaturePenalty = 0.0f;
        HumidityContrast = 1.0f;
        HumidityOffset = 0.5f;
        break;

    case ECosmicBiomeType::Ice:
        // Hielo - frío extremo
        TemperatureFrequency = 0.0005f;
        HumidityFrequency = 0.01f;
        HumidityOctaves = 4;
        LatitudeEffect = 1.8f;
        AltitudeTemperaturePenalty = 0.95f;
        HumidityContrast = 1.2f;
        HumidityOffset = 0.1f;
        break;

    case ECosmicBiomeType::Cratered:
    default:
        // Lunar - temperaturas extremas
        TemperatureFrequency = 0.03f;
        HumidityFrequency = 0.001f;
        HumidityOctaves = 1;
        LatitudeEffect = 2.0f;
        AltitudeTemperaturePenalty = 0.5f;
        HumidityContrast = 1.0f;
        HumidityOffset = -0.5f;
        break;
    }
    UE_LOG(LogTemp, Error, TEXT("FALLÓ la creación de la malla!"));
}

void UCosmicNoiseSettings::UpdateNoiseFromSimple()
{
    // 2. Limpiar capas existentes
    NoiseLayers.Empty();

    FCosmicNoiseTypes Layer;

    // Tipo 
    Layer.NoiseType = ECosmicNoiseType::Simplex;
    Layer.FractalType = ECosmicFractalType::FBM;

    // Frecuencia
    // Mas Smoothness = formas mas grandes (menor frecuencia)
    Layer.Frequency = FMath::Lerp(0.01f, 10.f, 1.f - Smoothness);

    // Octavas
    // Mas Detail = mas octavas
    Layer.Octaves = FMath::RoundToInt(FMath::Lerp(2.f, 12.f, Detail));

    // Lacunarity 
    // Mas Roughness = mas separacion entre octavas
    Layer.Lacunarity = FMath::Lerp(1.8f, 2.3f, Roughness);

    // Persistence (Gain en FastNoiseLite) 
    // Mas Mountainous = mas energia en altas frecuencias
    Layer.Persistence = FMath::Lerp(0.4f, 0.65f, Mountainous);

    // Amplitud
    // Control principal de altura
    Layer.Amplitude = MaxMountainHeight;

    NoiseLayers.Add(Layer);

    // Warp proporcional a la altura máxima
    DomainWarpStrength = MaxMountainHeight * FMath::Lerp(0.02f, 0.15f, Roughness);

    // Frecuencia alineada con la base
    DomainWarpFrequency = FMath::Lerp(0.001f, 0.004f, Roughness);
}

#if WITH_EDITOR
void UCosmicNoiseSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    // Obtener nombre de la propiedad
    FName PropertyName = (PropertyChangedEvent.Property != nullptr)
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    // Solo actualizar si estamos en modo simple
    if (!bUseAdvancedSettings)
    {
        if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicNoiseSettings, BiomeType))
        {
            UpdateBiomesFromSimple();
        }
        else if (
            PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicNoiseSettings, Seed) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicNoiseSettings, MaxMountainHeight) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicNoiseSettings, Mountainous) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicNoiseSettings, Roughness) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicNoiseSettings, Detail) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicNoiseSettings, Smoothness)
            )
        {
            UpdateNoiseFromSimple();
        }
    }

    Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
