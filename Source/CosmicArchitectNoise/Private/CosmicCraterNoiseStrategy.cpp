// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicCraterNoiseStrategy.h"
#include "CosmicEarthLikeNoiseStrategy.h"

void FCosmicCraterNoiseStrategy::Initialize(int32 InSeed, FCosmicNoiseLayer InLayerParameters, FCosmicNoiseBiomeParameters InBiomeParameters, FCosmicNoiseCraterParameters InCraterParameters)
{
    Seed = InSeed;
    LayerParameters = InLayerParameters;
    BiomeParameters = InBiomeParameters;
    CraterParameters = InCraterParameters;

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

    //Ruido para los cráteres
    CraterNoise.SetSeed(Seed + 512);
    CraterNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);

    CraterNoise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean);
    CraterNoise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance);
    CraterNoise.SetFrequency(CraterParameters.CraterFrequency);

    /*if (CraterParameters.CraterOctaves > 1)
    {
        CraterNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
        CraterNoise.SetFractalOctaves(CraterParameters.CraterOctaves);
        CraterNoise.SetFractalLacunarity(CraterParameters.CraterLacunarity);
        CraterNoise.SetFractalGain(CraterParameters.CraterPersistence);
    }*/
    CraterNoise.SetFractalType(FastNoiseLite::FractalType_None);
}

void FCosmicCraterNoiseStrategy::EvaluatePoint(const FVector& NoiseDir, float& OutHeight, FLinearColor& OutColor) const
{
    float X = NoiseDir.X;
    const float Y = NoiseDir.Y;
    const float Z = NoiseDir.Z;

    // ALTURA BASE
    float BaseNoise = Noise.GetNoise(X, Y, Z); // [-1,1]
    float Height = BaseNoise * LayerParameters.Amplitude;

    // CRATERES
    float FinalCraterHeight = 0.0f;
    float FreqScale = 1.0f;
    float AmpScale = 1.0f;

    for (int32 i = 0; i < CraterParameters.CraterOctaves; ++i) {
        float OffsetX = (X * FreqScale) + (i * 133.7f);
        float OffsetY = (Y * FreqScale) + (i * 133.7f);
        float OffsetZ = (Z * FreqScale) + (i * 133.7f);
        float RawCrater = FMath::Abs(CraterNoise.GetNoise(OffsetX, OffsetY, OffsetZ)); //Altura inicial del ruido celular

        float SizeVariation = FMath::Lerp(0.6f, 1.4f, (BaseNoise + 1.0f) * 0.5f);
        float DynamicRadius = CraterParameters.CraterRadiusMultiplier * SizeVariation;

        // Se escala la cavidad
        float D = RawCrater / FMath::Max(DynamicRadius, 0.01f);
        float Cavity = (D * D) - 1.0f;
        Cavity = FMath::Clamp(Cavity, -1.0f, 0.0f);


        //Se afilan los bordes
        float RimDistance = FMath::Abs(D - 1.0f);
        float Rim = 1.0f - (RimDistance * CraterParameters.CraterRimSharpness);
        Rim = FMath::Clamp(Rim, 0.0f, 1.0f);
        Rim = Rim * Rim * (3.0f - 2.0f * Rim);

        float CurrentDepth = CraterParameters.CraterDepth * AmpScale;
        float ThisCrater = (Cavity * CurrentDepth) + (Rim * (CurrentDepth * CraterParameters.CraterRimHeight));

        // Suelo del cráter
        if (ThisCrater < -CurrentDepth + (CraterParameters.CraterFloorHeight * AmpScale))
        {
            ThisCrater = -CurrentDepth + (CraterParameters.CraterFloorHeight * AmpScale);
        }

        FinalCraterHeight += ThisCrater;

        FreqScale *= CraterParameters.CraterLacunarity; // Suele ser 2.0 o 2.5
        AmpScale *= CraterParameters.CraterPersistence;   // Suele ser 0.4 o 0.5
    }

    // Restar/Sumar al terreno base
    Height += FinalCraterHeight;

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
