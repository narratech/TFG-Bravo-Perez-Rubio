// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicNoiseSettings.h"
#include "CosmicNoiseTypes.h"

UCosmicNoiseSettings::UCosmicNoiseSettings()
{
    Seed = 1337;
    MaxMountainHeight = 3000.f;
    Mountainous = 0.6f;
    Roughness = 0.4f;
    Detail = 0.7f;
    Smoothness = 0.5f;

    UpdateAdvancedFromSimple();
}

void UCosmicNoiseSettings::UpdateAdvancedFromSimple()
{
    if (bIsUpdatingAdvanced) return;
    bIsUpdatingAdvanced = true;

    // 2. Limpiar capas existentes
    NoiseLayers.Empty();

    // 3. Capa base (terreno principal)
    FCosmicNoiseTypes BaseLayer;
    BaseLayer.NoiseType = ECosmicNoiseType::Perlin;
    BaseLayer.FractalType = ECosmicFractalType::FBM;
    BaseLayer.Frequency = FMath::Lerp(0.0005f, 0.002f, Smoothness);
    BaseLayer.Octaves = 4;
    BaseLayer.Lacunarity = 2.0f;
    BaseLayer.Persistence = 0.5f;
    BaseLayer.Amplitude = MaxMountainHeight * 0.5f;
    NoiseLayers.Add(BaseLayer);

    // 4. Capa de montañas
    FCosmicNoiseTypes MountainLayer;
    MountainLayer.NoiseType = ECosmicNoiseType::Ridged;
    MountainLayer.FractalType = ECosmicFractalType::Ridged;
    MountainLayer.Frequency = 0.003f;
    MountainLayer.Octaves = FMath::RoundToInt(FMath::Lerp(3.f, 6.f, Mountainous));
    MountainLayer.Lacunarity = 2.0f;
    MountainLayer.Persistence = 0.5f;
    MountainLayer.Amplitude = Mountainous * MaxMountainHeight * 0.8f;
    NoiseLayers.Add(MountainLayer);

    // 5. Capa de detalle (influenciada por Roughness)
    FCosmicNoiseTypes DetailLayer;
    DetailLayer.NoiseType = ECosmicNoiseType::Simplex;
    DetailLayer.FractalType = ECosmicFractalType::FBM;
    DetailLayer.Frequency = FMath::Lerp(0.01f, 0.03f, Roughness);
    DetailLayer.Octaves = FMath::RoundToInt(FMath::Lerp(2.f, 5.f, Detail));
    DetailLayer.Lacunarity = 2.0f;
    DetailLayer.Persistence = 0.5f;
    DetailLayer.Amplitude = Detail * 300.0f;
    NoiseLayers.Add(DetailLayer);

    // 6. Domain Warp (basado en Roughness)
    bUseDomainWarp = Roughness > 0.3f;
    DomainWarpStrength = FMath::Lerp(200.0f, 2000.0f, Roughness);
    DomainWarpFrequency = FMath::Lerp(0.0005f, 0.002f, Roughness);

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
