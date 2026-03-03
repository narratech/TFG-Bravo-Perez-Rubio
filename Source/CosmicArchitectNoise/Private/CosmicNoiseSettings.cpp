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
    BaseLayer.Frequency = FMath::Lerp(25.f, 100.f, 1.f - Smoothness);
    BaseLayer.Octaves = 4;
    BaseLayer.Lacunarity = 2.0f;
    BaseLayer.Persistence = 0.5f;
    BaseLayer.Amplitude = 100 * MaxMountainHeight * 0.4f;
    NoiseLayers.Add(BaseLayer);

    // 4. Capa de montañas
    FCosmicNoiseTypes MountainLayer;
    MountainLayer.NoiseType = ECosmicNoiseType::Ridged;
    MountainLayer.FractalType = ECosmicFractalType::Ridged;
    MountainLayer.Frequency = FMath::Lerp(100.f, 150.f, Mountainous);
    MountainLayer.Octaves = FMath::RoundToInt(FMath::Lerp(3.f, 6.f, Mountainous));
    MountainLayer.Lacunarity = 2.0f;
    MountainLayer.Persistence = 0.5f;
    MountainLayer.Amplitude = 100 * Mountainous * MaxMountainHeight * 0.5f;
    NoiseLayers.Add(MountainLayer);

    // 5. Capa de detalle (influenciada por Roughness)
    FCosmicNoiseTypes DetailLayer;
    DetailLayer.NoiseType = ECosmicNoiseType::Simplex;
    DetailLayer.FractalType = ECosmicFractalType::FBM;
    DetailLayer.Frequency = FMath::Lerp(450.f, 2000.f, Roughness);
    DetailLayer.Octaves = FMath::RoundToInt(FMath::Lerp(2.f, 5.f, Detail));
    DetailLayer.Lacunarity = 2.0f;
    DetailLayer.Persistence = 0.4f;
    DetailLayer.Amplitude = MaxMountainHeight * Detail * (1 - Smoothness + 0.01) * 2;
    NoiseLayers.Add(DetailLayer);

    // 6. Domain Warp (basado en Roughness)
    bUseDomainWarp = Roughness > 0.25f;
    DomainWarpStrength = FMath::Lerp(200.0f, 2000.0f, Roughness);
    DomainWarpFrequency = FMath::Lerp(10.f, 40.f, Roughness);

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
