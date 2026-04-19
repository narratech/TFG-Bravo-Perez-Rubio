// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicDefaultNoiseStrategy.h"

void FCosmicDefaultNoiseStrategy::Initialize(int32 InSeed, FCosmicNoiseLayer InLayerParameters, FCosmicNoiseBiomeParameters InBiomeParameters)
{
    Seed = InSeed;
    LayerParameters = InLayerParameters;
    BiomeParameters = InBiomeParameters;

    Noise.SetSeed(Seed);
    switch (LayerParameters.NoiseType) {
    case ECosmicNoiseType::Perlin: Noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin); break;
    case ECosmicNoiseType::Simplex:
    case ECosmicNoiseType::Ridged: Noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2); break;
    case ECosmicNoiseType::Cellular: Noise.SetNoiseType(FastNoiseLite::NoiseType_Cellular); break;
    case ECosmicNoiseType::Value: Noise.SetNoiseType(FastNoiseLite::NoiseType_Value); break;
    }
    switch (LayerParameters.FractalType) {
    case ECosmicFractalType::None: Noise.SetFractalType(FastNoiseLite::FractalType_None); break;
    case ECosmicFractalType::FBM: Noise.SetFractalType(FastNoiseLite::FractalType_FBm); break;
    case ECosmicFractalType::Ridged: Noise.SetFractalType(FastNoiseLite::FractalType_Ridged); break;
    case ECosmicFractalType::PingPong: Noise.SetFractalType(FastNoiseLite::FractalType_PingPong); break;
    }
    Noise.SetFrequency(LayerParameters.Frequency);
    Noise.SetFractalOctaves(LayerParameters.Octaves);
    Noise.SetFractalLacunarity(LayerParameters.Lacunarity);
    Noise.SetFractalGain(LayerParameters.Persistence);

    HumidityNoise.SetSeed(Seed + 128);
    HumidityNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    HumidityNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    HumidityNoise.SetFrequency(BiomeParameters.HumidityFrequency);
    HumidityNoise.SetFractalOctaves(BiomeParameters.HumidityOctaves);

    TempNoise.SetSeed(Seed + 256);
    TempNoise.SetFrequency(BiomeParameters.TemperatureFrequency);
}

void FCosmicDefaultNoiseStrategy::EvaluatePoint(const FVector& NoiseDir, float& OutHeight, FLinearColor& OutColor) const
{
    float X = NoiseDir.X;
    const float Y = NoiseDir.Y;
    const float Z = NoiseDir.Z;

    // ALTURA BASE
    float BaseNoise = Noise.GetNoise(X, Y, Z); // [-1,1]
    float Height = BaseNoise * LayerParameters.Amplitude;

    // HUMEDAD
    float RawHum = HumidityNoise.GetNoise(X, Y, Z);
    float Humidity = (RawHum + 1.0f) * 0.5f; // [0,1]

    Humidity = (Humidity + BiomeParameters.HumidityOffset);
    Humidity = FMath::Clamp(
        (Humidity - 0.5f) * BiomeParameters.HumidityContrast + 0.5f,
        0.0f,
        1.0f
    );

    // TEMPERATURA
    float Latitude = FMath::Abs(Z);

    float BaseTemp = 1.0f - (Latitude * BiomeParameters.LatitudeEffect);
    float TempNoiseVal = TempNoise.GetNoise(X, Y, Z) * 0.2f;

    float Temperature = FMath::Clamp(BaseTemp + TempNoiseVal, 0.0f, 1.0f);

    // MODIFICACION POR BIOMA 
    // Ejemplo: zonas humedas mas suaves, zonas secas mas abruptas

    float BiomeInfluence = FMath::Lerp(0.8f, 1.2f, Humidity);
    Height *= BiomeInfluence;

    // Penalizacion por altitud en temperatura
    float AltitudeNormalized = FMath::Clamp(
        Height / FMath::Max(LayerParameters.Amplitude, 1.0f),
        0.0f,
        1.0f
    );

    float VisualTemp = FMath::Clamp(
        Temperature - (AltitudeNormalized * BiomeParameters.AltitudeTemperaturePenalty),
        0.0f,
        1.0f
    );

    // SALIDA
    OutHeight = Height;

    // R = altura normalizada
    // G = temperatura visual
    // B = humedad
    // A = libre 
    OutColor = FLinearColor(
        AltitudeNormalized,
        VisualTemp,
        Humidity,
        0.0f
    );
}
