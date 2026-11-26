// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicCraterNoiseSettings.h"
#include "CosmicCraterNoiseStrategy.h"

UCosmicCraterNoiseSettings::UCosmicCraterNoiseSettings()
{
    Seed = 1337;
    HeightNormalizationScale = 1.0f;

    // 1. Macro planetary base crust layer
    LayerParameters.NoiseType = ECosmicNoiseType::Simplex;
    LayerParameters.FractalType = ECosmicFractalType::FBM;
    LayerParameters.Frequency = 3.2f;
    LayerParameters.Octaves = 6;
    LayerParameters.Lacunarity = 2.1f;
    LayerParameters.Persistence = 0.5f;
    LayerParameters.Amplitude = 6000.0f;

    // 2. Multi-scale impact craters (unit-sphere calibrated defaults)
    CraterParameters.CraterFrequency = 3.5f;
    CraterParameters.CraterDepth = 8000.0f;
    CraterParameters.CraterOctaves = 5;
    CraterParameters.CraterDensity = 0.55f;
    CraterParameters.CraterRadiusMultiplier = 0.24f;
    CraterParameters.CraterRimHeight = 0.35f;
    CraterParameters.CraterRimSharpness = 9.0f;
    CraterParameters.CraterFloorHeight = 0.55f;
    CraterParameters.CentralPeakHeight = 0.35f;
    CraterParameters.CentralPeakRadius = 0.22f;
    CraterParameters.EjectaStrength = 0.45f;
    CraterParameters.CraterDistortion = 0.45f;
    CraterParameters.CraterLacunarity = 2.25f;
    CraterParameters.CraterPersistence = 0.52f;
    CraterParameters.CraterNoiseBreakup = 0.55f;
    CraterParameters.MariaThreshold = 0.38f;

    // 3. Hybrid ridged multifractal mountain chains (Amplitude is relative to LayerParameters.Amplitude)
    RidgeLayer.Frequency = 8.0f;
    RidgeLayer.Octaves = 6;
    RidgeLayer.Lacunarity = 2.1f;
    RidgeLayer.Persistence = 0.52f;
    RidgeLayer.Amplitude = 0.65f;

    // 4. High-frequency micro-detail (Amplitude is relative to LayerParameters.Amplitude)
    DetailLayer.Frequency = 80.0f;
    DetailLayer.Octaves = 4;
    DetailLayer.Lacunarity = 2.0f;
    DetailLayer.Persistence = 0.45f;
    DetailLayer.Amplitude = 0.12f;

    // 5. Geological terrain shaping & crater weathering
    TerrainParameters.RidgePeakSharpness = 1.65f;
    TerrainParameters.RidgeWeightedStrength = 0.80f;
    TerrainParameters.MariaBasinDepthScale = 0.55f;
    TerrainParameters.MariaRoughnessFactor = 0.22f;
    TerrainParameters.MariaCraterFlooding = 0.45f;
    TerrainParameters.CraterFloorFlattening = 0.65f;
    TerrainParameters.CraterAgeVariation = 0.45f;
    TerrainParameters.InnerWallTerracing = 0.25f;
    TerrainParameters.EjectaRayBreakup = 0.55f;
    TerrainParameters.CraterBiomeInfluence = 0.15f;

    // 6. Domain Warping (Inigo Quilez crustal folding)
    DomainWarpParams.bUseDomainWarp = true;
    DomainWarpParams.DomainWarpStrength = 0.25f;
    DomainWarpParams.DomainWarpFrequency = 1.4f;
    DomainWarpParams.DomainWarpOctaves = 3;

    // 7. Planetary Climatology & Biomes (unit-sphere calibrated)
    BiomeParameters.TemperatureFrequency = 3.5f;
    BiomeParameters.LatitudeEffect = 0.55f;
    BiomeParameters.AltitudeTemperaturePenalty = 0.55f;
    BiomeParameters.HumidityFrequency = 12.0f;
    BiomeParameters.HumidityOctaves = 4;
    BiomeParameters.HumidityContrast = 1.45f;
    BiomeParameters.HumidityOffset = 0.0f;
}

TSharedPtr<ICosmicNoiseStrategy> UCosmicCraterNoiseSettings::CreateStrategy() const
{
    auto Strategy = MakeShared<FCosmicCraterNoiseStrategy>();
    Strategy->Initialize(*this);
    return Strategy;
}