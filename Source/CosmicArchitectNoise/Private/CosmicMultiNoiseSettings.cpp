// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicMultiNoiseSettings.h"
#include "CosmicMultiNoiseStrategy.h"

UCosmicMultiNoiseSettings::UCosmicMultiNoiseSettings()
{
    Seed = 1337;
    HeightNormalizationScale = 1.0f;

    // Continentalness: Low frequency defining major oceanic basins and continental shields
    ContinentalLayer.Frequency = 0.75f;
    ContinentalLayer.Octaves = 5;
    ContinentalLayer.Lacunarity = 2.0f;
    ContinentalLayer.Persistence = 0.5f;
    ContinentalLayer.Amplitude = 3500.0f;

    // Erosion: Medium frequency defining geological age and terrain smoothing
    ErosionLayer.Frequency = 1.5f;
    ErosionLayer.Octaves = 4;
    ErosionLayer.Lacunarity = 2.0f;
    ErosionLayer.Persistence = 0.5f;
    ErosionLayer.Amplitude = 1.0f;

    // Peaks & Valleys: High frequency alpine ridged mountains and deep gorges
    PeaksValleysLayer.Frequency = 3.0f;
    PeaksValleysLayer.Octaves = 6;
    PeaksValleysLayer.Lacunarity = 2.0f;
    PeaksValleysLayer.Persistence = 0.55f;
    PeaksValleysLayer.Amplitude = 4500.0f;

    // Detail: High frequency micro-relief (gravel, rock faces, boulders)
    DetailLayer.Frequency = 14.0f;
    DetailLayer.Octaves = 3;
    DetailLayer.Lacunarity = 2.0f;
    DetailLayer.Persistence = 0.45f;
    DetailLayer.Amplitude = 200.0f;

    // Multi-Noise specialized parameters
    MultiNoiseParams.SeaLevelThreshold = 0.45f;
    MultiNoiseParams.OceanDepthScale = 0.7f;
    MultiNoiseParams.ContinentalShelfWidth = 0.08f;
    MultiNoiseParams.InlandPlateauBoost = 0.35f;
    MultiNoiseParams.HighErosionFlattening = 0.15f;
    MultiNoiseParams.ErosionContrast = 1.4f;
    MultiNoiseParams.MountainPeakSharpness = 2.2f;
    MultiNoiseParams.ValleyDepthMultiplier = 0.85f;
    MultiNoiseParams.TerracingStrength = 0.0f;
    MultiNoiseParams.TerraceSteps = 6.0f;

    // Domain Warping: Enabled with organic flow
    DomainWarpParams.bUseDomainWarp = true;
    DomainWarpParams.DomainWarpStrength = 0.25f;
    DomainWarpParams.DomainWarpFrequency = 1.2f;
    DomainWarpParams.DomainWarpOctaves = 3;

    // Climatology & Biomes
    BiomeParameters.TemperatureFrequency = 0.6f;
    BiomeParameters.LatitudeEffect = 1.0f;
    BiomeParameters.AltitudeTemperaturePenalty = 0.65f;
    BiomeParameters.HumidityFrequency = 1.0f;
    BiomeParameters.HumidityOctaves = 4;
    BiomeParameters.HumidityContrast = 1.4f;
    BiomeParameters.HumidityOffset = 0.0f;

    // Orographic Climatology: Real atmospheric wind cells & mountain rain shadow
    OrographicParams.bEnableOrographicEffect = true;
    OrographicParams.bUsePlanetaryZonalWinds = true;
    OrographicParams.PrevailingWindDirection = FVector(1.0f, 0.0f, 0.0f);
    OrographicParams.OrographicLiftStrength = 1.2f;
    OrographicParams.RainShadowStrength = 1.5f;
    OrographicParams.WindwardSampleOffset = 0.015f;
    OrographicParams.RainShadowDistance = 0.06f;
}

TSharedPtr<ICosmicNoiseStrategy> UCosmicMultiNoiseSettings::CreateStrategy() const
{
    auto Strategy = MakeShared<FCosmicMultiNoiseStrategy>();

    Strategy->Initialize(
        Seed,
        HeightNormalizationScale,
        ContinentalLayer,
        ErosionLayer,
        PeaksValleysLayer,
        DetailLayer,
        MultiNoiseParams,
        DomainWarpParams,
        BiomeParameters,
        OrographicParams
    );

    return Strategy;
}
