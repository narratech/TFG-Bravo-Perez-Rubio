// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.


#include "CosmicRealisticNoiseStrategy.h"

void FCosmicRealisticNoiseStrategy::Initialize(
    int32 InSeed,
    float InHeightNormalizationScale,
    FCosmicNoiseBiomeParameters InBiomeParameters,
    FCosmicNoiseDataLayer InContinental,
    FCosmicNoiseDataLayer InMountain,
    FCosmicNoiseDataLayer InHill,
    FCosmicNoiseDataLayer InDiffer,
    FCosmicNoiseDataLayer InRiver)
{
    Seed = InSeed;
    HeightNormalizationScale = InHeightNormalizationScale;
    BiomeParameters = InBiomeParameters;

    ContinentalLayer = InContinental;
    MountainLayer = InMountain;
    HillLayer = InHill;
    DifferLayer = InDiffer;

    auto SetupSimplexFBM = [&](FastNoiseLite& Noise, const FCosmicNoiseDataLayer& Layer, int32 Offset)
        {
            Noise.SetSeed(Seed + Offset);
            Noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
            Noise.SetFractalType(FastNoiseLite::FractalType_FBm);
            Noise.SetFrequency(Layer.Frequency);
            Noise.SetFractalOctaves(Layer.Octaves);
            Noise.SetFractalLacunarity(Layer.Lacunarity);
            Noise.SetFractalGain(Layer.Persistence);
        };

    // Continents (base)
    SetupSimplexFBM(ContinentalNoise, ContinentalLayer, 0);

    // Hills (smooth FBM)
    SetupSimplexFBM(HillNoise, HillLayer, 200);

    // Detail (ValueCubic FBM)
    DifferNoise.SetSeed(Seed + 300);
    DifferNoise.SetNoiseType(FastNoiseLite::NoiseType_ValueCubic);
    DifferNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    DifferNoise.SetFrequency(DifferLayer.Frequency);
    DifferNoise.SetFractalOctaves(DifferLayer.Octaves);
    DifferNoise.SetFractalLacunarity(DifferLayer.Lacunarity);
    DifferNoise.SetFractalGain(DifferLayer.Persistence);

    // Mountains (RIDGED)
    MountainNoise.SetSeed(Seed + 100);
    MountainNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    MountainNoise.SetFractalType(FastNoiseLite::FractalType_Ridged);
    MountainNoise.SetFrequency(MountainLayer.Frequency);
    MountainNoise.SetFractalOctaves(MountainLayer.Octaves);
    MountainNoise.SetFractalLacunarity(MountainLayer.Lacunarity);
    MountainNoise.SetFractalGain(MountainLayer.Persistence);

    // (Optional) Climate - if needed for color, keep them
    SetupSimplexFBM(HumidityNoise, { BiomeParameters.HumidityFrequency, BiomeParameters.HumidityOctaves, 2.0f, 0.5f, 1.0f }, 500);
    SetupSimplexFBM(TempNoise, { BiomeParameters.TemperatureFrequency, 3, 2.0f, 0.5f, 1.0f }, 600);
}

void FCosmicRealisticNoiseStrategy::EvaluatePoint(
    const FVector& NoiseDir,
    float& OutHeight,
    FLinearColor& OutColor) const
{
    const float X = NoiseDir.X;
    const float Y = NoiseDir.Y;
    const float Z = NoiseDir.Z;

    // ------------------ CONTINENTAL LAYER (ocean vs land) ------------------
    float ContinentRaw = ContinentalNoise.GetNoise(X, Y, Z);
    float Continent = (ContinentRaw + 1.0f) * 0.5f;          // [0,1]
    float BaseHeight = (Continent - 0.5f) * ContinentalLayer.Amplitude;

    float OceanMask = FMath::SmoothStep(0.4f, 0.5f, Continent);
    float LandMask = 1.0f - OceanMask;

    // ------------------ RELIEF MASK (mountain vs hill) ------------------
    // Get a mask value between 0 and 1 from DifferNoise
    float DiffRaw = DifferNoise.GetNoise(X, Y, Z);
    // Normalize from [-1,1] or [0,1] depending on noise. ValueCubic usually gives [-1,1].
    float DiffMask = (DiffRaw + 1.0f) * 0.5f;   // now [0,1]
    // Optional: apply contrast for more defined zones
    DiffMask = FMath::Clamp((DiffMask - 0.5f) * 2.0f + 0.5f, 0.0f, 1.0f); // optional contrast

    // ------------------ MOUNTAIN AND HILL GENERATION ------------------
    // Mountains (Ridged)
    float MountainRaw = MountainNoise.GetNoise(X, Y, Z);
    float Mountain = FMath::Max(0.0f, MountainRaw);    // Ridged already gives approx [0,1]
    float MountainHeight = Mountain * MountainLayer.Amplitude;

    // Hills (smooth FBM)
    float HillRaw = HillNoise.GetNoise(X, Y, Z);
    float Hill = (HillRaw + 1.0f) * 0.5f;               // [0,1]
    float HillHeight = Hill * HillLayer.Amplitude;

    // Blend according to DiffMask: where DiffMask is high -> mountains, low -> hills
    float ReliefHeight = FMath::Lerp(HillHeight, MountainHeight, DiffMask);

    // ------------------ FINAL COMBINATION ------------------
    // Only apply relief over dry land
    float Height = BaseHeight + (ReliefHeight * LandMask);

    // Sink oceans (optional)
    Height = FMath::Lerp(Height, -ContinentalLayer.Amplitude * 0.6f, OceanMask);

    // (Optional) Add a small extra detail noise if desired, but DifferLayer is no longer used for that.
    // If microdetail is desired, another layer can be added separately, but this is sufficient.

    // ------------------ NORMALIZATION AND COLOR ------------------
    float TrueMinHeight = -0.6f * ContinentalLayer.Amplitude;
    float TrueMaxHeight = 0.5f * ContinentalLayer.Amplitude
        + FMath::Max(MountainLayer.Amplitude, HillLayer.Amplitude); // the greater of the two
    float NormalizedMin = TrueMinHeight * HeightNormalizationScale;
    float NormalizedMax = TrueMaxHeight * HeightNormalizationScale;
    float AltitudeNormalized = (Height - NormalizedMin) / (NormalizedMax - NormalizedMin);
    AltitudeNormalized = FMath::Clamp(AltitudeNormalized, 0.0f, 1.0f);

    // Climate calculation (if needed for color)
    float RawHum = HumidityNoise.GetNoise(X, Y, Z);
    float Humidity = (RawHum + 1.0f) * 0.5f;
    Humidity = FMath::Clamp((Humidity + BiomeParameters.HumidityOffset - 0.5f) * BiomeParameters.HumidityContrast + 0.5f, 0.0f, 1.0f);

    float Latitude = FMath::Abs(Z);
    float BaseTemp = 1.0f - (Latitude * BiomeParameters.LatitudeEffect);
    float TempVar = TempNoise.GetNoise(X, Y, Z) * 0.2f;
    float Temperature = FMath::Clamp(BaseTemp + TempVar, 0.0f, 1.0f);
    float VisualTemp = FMath::Clamp(Temperature - (AltitudeNormalized * BiomeParameters.AltitudeTemperaturePenalty), 0.0f, 1.0f);

    OutHeight = Height;
    OutColor = FLinearColor(AltitudeNormalized, VisualTemp, Humidity, 1.0f);
}
