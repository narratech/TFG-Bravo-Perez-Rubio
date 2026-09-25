// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicDefaultNoiseStrategy.h"
#include "Math/UnrealMathUtility.h"

void FCosmicDefaultNoiseStrategy::Initialize(
    int32 InSeed,
    const FCosmicNoiseLayer& InLayerParameters,
    const FCosmicNoiseBiomeParameters& InBiomeParameters,
    float InSeaLevel,
    float InOceanDepthScale,
    float InContinentScale,
    bool bInEnableMountainRidges,
    float InMountainSharpness,
    float InMountainRidgeStrength,
    float InTerraceSteps,
    float InHeightNormalizationScale)
{
    Seed = InSeed;
    LayerParameters = InLayerParameters;
    BiomeParameters = InBiomeParameters;
    SeaLevel = InSeaLevel;
    OceanDepthScale = InOceanDepthScale;
    ContinentScale = InContinentScale;
    bEnableMountainRidges = bInEnableMountainRidges;
    MountainSharpness = InMountainSharpness;
    MountainRidgeStrength = InMountainRidgeStrength;
    TerraceSteps = InTerraceSteps;
    HeightNormalizationScale = InHeightNormalizationScale;

    // 1. Base Primary Noise Layer
    Noise.SetSeed(Seed);
    switch (LayerParameters.NoiseType)
    {
    case ECosmicNoiseType::Perlin:   Noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin); break;
    case ECosmicNoiseType::Simplex:  Noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2); break;
    case ECosmicNoiseType::Cellular: Noise.SetNoiseType(FastNoiseLite::NoiseType_Cellular); break;
    case ECosmicNoiseType::Value:    Noise.SetNoiseType(FastNoiseLite::NoiseType_Value); break;
    case ECosmicNoiseType::Ridged:   Noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2); break;
    default:                         Noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2); break;
    }

    switch (LayerParameters.FractalType)
    {
    case ECosmicFractalType::None:     Noise.SetFractalType(FastNoiseLite::FractalType_None); break;
    case ECosmicFractalType::FBM:      Noise.SetFractalType(FastNoiseLite::FractalType_FBm); break;
    case ECosmicFractalType::Ridged:   Noise.SetFractalType(FastNoiseLite::FractalType_Ridged); break;
    case ECosmicFractalType::PingPong: Noise.SetFractalType(FastNoiseLite::FractalType_PingPong); break;
    default:                           Noise.SetFractalType(FastNoiseLite::FractalType_FBm); break;
    }

    Noise.SetFrequency(LayerParameters.Frequency);
    Noise.SetFractalOctaves(FMath::Clamp(LayerParameters.Octaves, 1, 8));
    Noise.SetFractalLacunarity(LayerParameters.Lacunarity);
    Noise.SetFractalGain(LayerParameters.Persistence);

    // 2. Fast Low-Octave Humidity Layer (only 2-3 octaves for hyper-fast execution)
    HumidityNoise.SetSeed(Seed + 128);
    HumidityNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    HumidityNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    HumidityNoise.SetFrequency(BiomeParameters.HumidityFrequency);
    HumidityNoise.SetFractalOctaves(FMath::Min(BiomeParameters.HumidityOctaves, 3));
    HumidityNoise.SetFractalGain(0.5f);
    HumidityNoise.SetFractalLacunarity(2.0f);
}

void FCosmicDefaultNoiseStrategy::EvaluatePoint(const FVector& NoiseDir, float& OutHeight, FLinearColor& OutColor) const
{
    const float X = NoiseDir.X;
    const float Y = NoiseDir.Y;
    const float Z = NoiseDir.Z;

    // 1. MACRO BASE ELEVATION (1 FastNoiseLite call)
    const float BaseNoise = Noise.GetNoise(X, Y, Z); // [-1, 1]

    // 2. HYPSOMETRIC CURVE: CONTINENTS & OCEAN BASINS
    const float Amp = FMath::Max(1.0f, LayerParameters.Amplitude);
    float Height = 0.0f;
    float LandMask = 0.0f;

    if (BaseNoise < SeaLevel)
    {
        // Ocean Basin: smooth transition from shallow coastal shelf to deep ocean floor
        const float ShelfStart = FMath::Max(-1.0f, SeaLevel - 0.15f);
        const float ShelfDenom = FMath::Max(0.001f, SeaLevel - ShelfStart);
        const float ShelfT = FMath::Clamp((BaseNoise - ShelfStart) / ShelfDenom, 0.0f, 1.0f);
        const float SlopeProfile = FMath::SmoothStep(0.0f, 1.0f, ShelfT);
        Height = FMath::Lerp(-Amp * OceanDepthScale, 0.0f, SlopeProfile);
        LandMask = 0.0f;
    }
    else
    {
        // Continental Landmass
        const float LandT = (BaseNoise - SeaLevel) / FMath::Max(0.001f, 1.0f - SeaLevel);
        LandMask = FMath::SmoothStep(0.0f, 0.04f, LandT);

        // Base continental shield elevation
        Height = LandT * Amp * ContinentScale;

        // 3. ANALYTICAL ALPINE MOUNTAIN RIDGES (ZERO extra noise cost!)
        if (bEnableMountainRidges && LandT > 0.08f)
        {
            // Slices and folds the continental signal to generate razor-sharp alpine crests
            const float MountainT = (LandT - 0.08f) / 0.92f;
            float Ridge = 1.0f - FMath::Abs(FMath::Sin(MountainT * PI * 1.5f));
            Ridge = FMath::Pow(Ridge, MountainSharpness);
            Height += Ridge * Amp * MountainRidgeStrength;
        }

        // 4. ANALYTICAL GEOLOGICAL TERRACES (ZERO extra noise cost!)
        if (TerraceSteps > 0.0f)
        {
            const float StepSize = 1000.0f / TerraceSteps;
            const float Stepped = FMath::FloorToFloat(Height / StepSize) * StepSize;
            const float StepFrac = (Height - Stepped) / StepSize;
            const float SmoothFrac = FMath::SmoothStep(0.15f, 0.85f, StepFrac);
            Height = Stepped + SmoothFrac * StepSize;
        }
    }

    // 5. CLIMATOLOGY & NORMALIZATION
    // Theoretical bounds for precise normalization
    const float TrueMinHeight = -Amp * OceanDepthScale;
    const float TrueMaxHeight = Amp * ContinentScale * (1.0f + (bEnableMountainRidges ? MountainRidgeStrength : 0.0f));
    const float Scale = FMath::Max(0.01f, HeightNormalizationScale);
    const float NormMin = TrueMinHeight * Scale;
    const float NormMax = TrueMaxHeight * Scale;

    const float AltitudeNormalized = FMath::Clamp(
        (Height - NormMin) / FMath::Max(0.001f, NormMax - NormMin),
        0.0f,
        1.0f
    );

    // Planetary solar latitudinal gradient (1.0 at equator, down to 0 at poles)
    const float Latitude = FMath::Abs(Z);
    const float BaseTemp = FMath::Clamp(1.0f - (Latitude * BiomeParameters.LatitudeEffect), 0.0f, 1.0f);

    // Adiabatic lapse rate (cooling with elevation -> snow on peaks)
    const float VisualTemp = FMath::Clamp(
        BaseTemp - (AltitudeNormalized * BiomeParameters.AltitudeTemperaturePenalty),
        0.0f,
        1.0f
    );

    // Fast humidity: oceanic proximity bonus + low-octave moisture sample
    const float OceanicMoisture = (1.0f - LandMask) * 0.95f + (1.0f - AltitudeNormalized) * 0.15f;
    const float RawHum = (HumidityNoise.GetNoise(X, Y, Z) + 1.0f) * 0.5f;
    float Humidity = FMath::Lerp(OceanicMoisture, RawHum, 0.45f);
    Humidity = FMath::Clamp((Humidity + BiomeParameters.HumidityOffset - 0.5f) * BiomeParameters.HumidityContrast + 0.5f, 0.0f, 1.0f);

    // OUTPUT
    OutHeight = Height;
    OutColor = FLinearColor(
        AltitudeNormalized, // R: Height / Hypsometry
        VisualTemp,         // G: Surface Temperature
        Humidity,           // B: Moisture / Vegetation
        1.0f                // A: Valid Alpha
    );
}
