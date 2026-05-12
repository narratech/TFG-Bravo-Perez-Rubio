// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicEarthLikeNoiseStrategy.h"

void FCosmicEarthLikeNoiseStrategy::Initialize(
    int32 InSeed,
    FCosmicNoiseBiomeParameters InBiomeParameters,
    FCosmicNoiseDataLayer InContinental,
    FCosmicNoiseDataLayer InMountain,
    FCosmicNoiseDataLayer InHill,
    FCosmicNoiseDataLayer InDetail,
    FCosmicNoiseDataLayer InRiver)
{
    Seed = InSeed;
    BiomeParameters = InBiomeParameters;

    ContinentalLayer = InContinental;
    MountainLayer = InMountain;
    HillLayer = InHill;
    DetailLayer = InDetail;
    RiverLayer = InRiver;

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

    // Continentes
    SetupSimplexFBM(ContinentalNoise, ContinentalLayer, 0);

    // Colinas
    SetupSimplexFBM(HillNoise, HillLayer, 200);

    // Detalle (ValueCubic)
    DetailNoise.SetSeed(Seed + 300);
    DetailNoise.SetNoiseType(FastNoiseLite::NoiseType_ValueCubic);
    DetailNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    DetailNoise.SetFrequency(DetailLayer.Frequency);
    DetailNoise.SetFractalOctaves(DetailLayer.Octaves);
    DetailNoise.SetFractalLacunarity(DetailLayer.Lacunarity);
    DetailNoise.SetFractalGain(DetailLayer.Persistence);

    // Montañas (RIDGED)
    MountainNoise.SetSeed(Seed + 100);
    MountainNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    MountainNoise.SetFractalType(FastNoiseLite::FractalType_Ridged);
    MountainNoise.SetFrequency(MountainLayer.Frequency);
    MountainNoise.SetFractalOctaves(MountainLayer.Octaves);
    MountainNoise.SetFractalLacunarity(MountainLayer.Lacunarity);
    MountainNoise.SetFractalGain(MountainLayer.Persistence);

    // Rios (CELLULAR)
    RiverNoise.SetSeed(Seed + 400);
    RiverNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
    RiverNoise.SetFractalType(FastNoiseLite::FractalType_None);
    RiverNoise.SetFrequency(RiverLayer.Frequency);
    RiverNoise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean);
    RiverNoise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance);

    // Clima
    SetupSimplexFBM(HumidityNoise, { BiomeParameters.HumidityFrequency, BiomeParameters.HumidityOctaves, 2.0f, 0.5f, 1.0f }, 500);
    SetupSimplexFBM(TempNoise, { BiomeParameters.TemperatureFrequency, 3, 2.0f, 0.5f, 1.0f }, 600);
}

void FCosmicEarthLikeNoiseStrategy::EvaluatePoint(
    const FVector& NoiseDir,
    float& OutHeight,
    FLinearColor& OutColor) const
{
    const float X = NoiseDir.X;
    const float Y = NoiseDir.Y;
    const float Z = NoiseDir.Z;

    // CONTINENTES
    float Continent = ContinentalNoise.GetNoise(X, Y, Z);
    Continent = (Continent + 1.0f) * 0.5f;

    float OceanMask = FMath::SmoothStep(0.4f, 0.5f, Continent);
    float LandMask = 1.0f - OceanMask;

    float BaseHeight = (Continent - 0.5f) * ContinentalLayer.Amplitude;

    // MONTANAS
    float Mountain = FMath::Max(0.0f, MountainNoise.GetNoise(X, Y, Z));
    float MountainMask = FMath::SmoothStep(0.4f, 0.6f, Continent);
    float MountainHeight = Mountain * MountainLayer.Amplitude;

    // COLINAS
    float Hills = HillNoise.GetNoise(X, Y, Z);
    float HillHeight = Hills * HillLayer.Amplitude;

    // BASE TERRAIN
    float Height = BaseHeight;

    Height = FMath::Lerp(Height, Height + HillHeight, LandMask);
    Height = FMath::Lerp(Height, Height + MountainHeight, MountainMask);

    // OCEANO
    Height = FMath::Lerp(Height, -ContinentalLayer.Amplitude * 0.5f, OceanMask);

    // RIOS
    float River = FMath::Abs(RiverNoise.GetNoise(X, Y, Z));
    float RiverMask = FMath::SmoothStep(0.0f, 0.05f, River);
    Height -= RiverMask * RiverLayer.Amplitude;

    // DETALLE
    float Detail = DetailNoise.GetNoise(X, Y, Z);
    Height += Detail * DetailLayer.Amplitude;

    // HUMEDAD
    float RawHum = HumidityNoise.GetNoise(X, Y, Z);
    float Humidity = (RawHum + 1.0f) * 0.5f;

    Humidity = (Humidity + BiomeParameters.HumidityOffset);
    Humidity = FMath::Clamp(
        (Humidity - 0.5f) * BiomeParameters.HumidityContrast + 0.5f,
        0.0f,
        1.0f
    );

    // TEMPERATURA
    float Latitude = FMath::Abs(Z);
    float BaseTemp = 1.0f - (Latitude * BiomeParameters.LatitudeEffect);
    float TempVar = TempNoise.GetNoise(X, Y, Z) * 0.2f;

    float Temperature = FMath::Clamp(BaseTemp + TempVar, 0.0f, 1.0f);

    // HIELO (polar)
    float IceMask = FMath::SmoothStep(0.6f, 1.0f, Latitude);

    // NORMALIZACION ALTURA
    float MaxHeight = FMath::Max(
        ContinentalLayer.Amplitude +
        MountainLayer.Amplitude +
        HillLayer.Amplitude,
        1.0f
    );

    float AltitudeNormalized = FMath::Clamp(Height / MaxHeight, 0.0f, 1.0f);

    float VisualTemp = FMath::Clamp(
        Temperature - (AltitudeNormalized * BiomeParameters.AltitudeTemperaturePenalty),
        0.0f,
        1.0f
    );

    // OUTPUT
    OutHeight = Height;

    OutColor = FLinearColor(
        AltitudeNormalized,  // R
        VisualTemp,          // G
        Humidity,            // B     
        IceMask              // A 
    );
}
