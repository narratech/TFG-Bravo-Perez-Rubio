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
    if (bIsUpdatingAdvanced) return;
    bIsUpdatingAdvanced = true;

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

    // Domain Warp (basado en Roughness)
    //bUseDomainWarp = Roughness > 0.25f;

    // Warp proporcional a la altura máxima
    DomainWarpStrength = MaxMountainHeight * FMath::Lerp(0.02f, 0.15f, Roughness);

    // Frecuencia alineada con la base
    DomainWarpFrequency = FMath::Lerp(0.001f, 0.004f, Roughness);

    bIsUpdatingAdvanced = false;
}

#if WITH_EDITOR
void UCosmicNoiseSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    // Prevenir recursión
    if (bIsUpdatingAdvanced)
    {
        Super::PostEditChangeProperty(PropertyChangedEvent);
        return;
    }

    // Obtener nombre de la propiedad
    FName PropertyName = (PropertyChangedEvent.Property != nullptr)
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    // Solo actualizar si estamos en modo simple
    if (!bUseAdvancedSettings)
    {
        // Lista de propiedades simples que disparan actualización
        bool bIsSimpleProperty = (
            PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicNoiseSettings, Seed) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicNoiseSettings, MaxMountainHeight) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicNoiseSettings, Mountainous) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicNoiseSettings, Roughness) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicNoiseSettings, Detail) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicNoiseSettings, Smoothness)
            );

        if (bIsSimpleProperty)
        {
            UpdateAdvancedFromSimple();
        }
    }

    Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
