#include "CosmicNoiseEvaluator.h"

FCosmicNoiseEvaluator::FCosmicNoiseEvaluator(FCosmicNoiseGenerationParameters InSettings)
    : Settings(InSettings)
{
    UpdateSettings(InSettings);
}

FCosmicNoiseEvaluator::FCosmicNoiseEvaluator() : Settings() {}

void FCosmicNoiseEvaluator::UpdateSettings(FCosmicNoiseGenerationParameters InSettings)
{
    Settings = InSettings;

    // 1. CONFIGURAR CLIMA Y WARP GLOBALES
    HumidityNoise.SetSeed(Settings.Seed);
    HumidityNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    HumidityNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    HumidityNoise.SetFrequency(Settings.HumidityFrequency * 100.0f);
    HumidityNoise.SetFractalOctaves(Settings.HumidityOctaves);

    TempNoise.SetSeed(Settings.Seed);
    TempNoise.SetFrequency(Settings.TemperatureFrequency * 100.0f);

    if (Settings.bUseDomainWarp)
    {
        WarpNoise.SetSeed(Settings.Seed + 1337);
        WarpNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        WarpNoise.SetFrequency(Settings.DomainWarpFrequency);
    }

    if (Settings.bIsCraterPlanet)
    {
        CraterNoise.SetSeed(Settings.Seed + 4242);
        CraterNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
        CraterNoise.SetFrequency(Settings.CraterFrequency);
        CraterNoise.SetFractalType(FastNoiseLite::FractalType_None);
        CraterNoise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean);
        CraterNoise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance);
    }

    BiomeNoises.Empty();
    // 2. CONFIGURAR MATRIZ DE BIOMAS
    BiomeNoises.SetNum(Settings.Biomes.Num());
    MaxPossibleHeight = 0.0f;

    for (int32 b = 0; b < Settings.Biomes.Num(); b++)
    {
        float BiomeMaxHeight = 0.0f;
        const FCosmicBiomeData& BiomeData = Settings.Biomes[b];

        BiomeNoises[b].Reserve(BiomeData.NoiseLayers.Num());

        for (int32 l = 0; l < BiomeData.NoiseLayers.Num(); l++)
        {
            const FCosmicNoiseTypes& Layer = BiomeData.NoiseLayers[l];
            FastNoiseLite Noise;

            Noise.SetSeed(Settings.Seed + (b * 100) + l);

            // TIPO DE RUIDO
            switch (Layer.NoiseType) {
            case ECosmicNoiseType::Perlin: Noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin); break;
            case ECosmicNoiseType::Simplex:
            case ECosmicNoiseType::Ridged: Noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2); break;
            case ECosmicNoiseType::Cellular: Noise.SetNoiseType(FastNoiseLite::NoiseType_Cellular); break;
            case ECosmicNoiseType::Value: Noise.SetNoiseType(FastNoiseLite::NoiseType_Value); break;
            }

            // TIPO DE FRACTAL
            switch (Layer.FractalType) {
            case ECosmicFractalType::None: Noise.SetFractalType(FastNoiseLite::FractalType_None); break;
            case ECosmicFractalType::FBM: Noise.SetFractalType(FastNoiseLite::FractalType_FBm); break;
            case ECosmicFractalType::Ridged: Noise.SetFractalType(FastNoiseLite::FractalType_Ridged); break;
            case ECosmicFractalType::PingPong: Noise.SetFractalType(FastNoiseLite::FractalType_PingPong); break;
            }

            Noise.SetFrequency(Layer.Frequency);
            Noise.SetFractalOctaves(Layer.Octaves);
            Noise.SetFractalLacunarity(Layer.Lacunarity);
            Noise.SetFractalGain(Layer.Persistence);

            BiomeNoises[b].Add(Noise);
            BiomeMaxHeight += Layer.Amplitude;
        }

        MaxPossibleHeight = FMath::Max(MaxPossibleHeight, BiomeMaxHeight);
    }

    if (MaxPossibleHeight == 0.0f) MaxPossibleHeight = 1000.0f;
}

void FCosmicNoiseEvaluator::EvaluatePoint(const FVector& NoiseDir, float& OutHeight, FLinearColor& OutColor)
{
    float X = NoiseDir.X;
    float Y = NoiseDir.Y;
    float Z = NoiseDir.Z;

    // 1. DOMAIN WARP
    if (Settings.bUseDomainWarp)
    {
        float WarpX = WarpNoise.GetNoise(X, Y, Z) * Settings.DomainWarpStrength;
        float WarpY = WarpNoise.GetNoise(X + 31.7f, Y + 17.3f, Z + 47.1f) * Settings.DomainWarpStrength;
        float WarpZ = WarpNoise.GetNoise(X + 59.2f, Y + 11.8f, Z + 23.4f) * Settings.DomainWarpStrength;
        X += WarpX; Y += WarpY; Z += WarpZ;
    }

    // 2. CLIMA MATEMÁTICO (Para elegir bioma)
    float RawHum = HumidityNoise.GetNoise(NoiseDir.X, NoiseDir.Y, NoiseDir.Z);
    float FinalHum = (RawHum + 1.0f) * 0.5f + Settings.HumidityOffset;
    FinalHum = FMath::Clamp((FinalHum - 0.5f) * Settings.HumidityContrast + 0.5f, 0.0f, 1.0f);

    float Latitude = FMath::Abs(NoiseDir.Z);
    float BaseTemp = 1.0f - (Latitude * Settings.LatitudeEffect);
    float TempVariance = TempNoise.GetNoise(NoiseDir.X, NoiseDir.Y, NoiseDir.Z) * 0.2f;
    float FinalTemp = FMath::Clamp(BaseTemp + TempVariance, 0.0f, 1.0f);

    // 3. SELECCIÓN DE BIOMA
    int32 ClosestBiomeIdx = -1;
    int32 SecondClosestIdx = -1;
    float MinDistSq = UE_BIG_NUMBER;
    float SecondMinDistSq = UE_BIG_NUMBER;

    for (int32 b = 0; b < Settings.Biomes.Num(); b++)
    {
        float dTemp = FinalTemp - Settings.Biomes[b].TargetTemperature;
        float dHum = FinalHum - Settings.Biomes[b].TargetHumidity;
        float DistSq = (dTemp * dTemp) + (dHum * dHum);

        if (DistSq < MinDistSq)
        {
            SecondMinDistSq = MinDistSq;
            SecondClosestIdx = ClosestBiomeIdx;
            MinDistSq = DistSq;
            ClosestBiomeIdx = b;
        }
        else if (DistSq < SecondMinDistSq)
        {
            SecondMinDistSq = DistSq;
            SecondClosestIdx = b;
        }
    }

    float FinalHeight = 0.0f;

    if (ClosestBiomeIdx != -1)
    {
        float Height1 = 0.0f;
        for (int32 l = 0; l < Settings.Biomes[ClosestBiomeIdx].NoiseLayers.Num(); l++) {
            Height1 += BiomeNoises[ClosestBiomeIdx][l].GetNoise(X, Y, Z) * Settings.Biomes[ClosestBiomeIdx].NoiseLayers[l].Amplitude;
        }

        if (SecondClosestIdx != -1)
        {
            float Height2 = 0.0f;
            for (int32 l = 0; l < Settings.Biomes[SecondClosestIdx].NoiseLayers.Num(); l++) {
                Height2 += BiomeNoises[SecondClosestIdx][l].GetNoise(X, Y, Z) * Settings.Biomes[SecondClosestIdx].NoiseLayers[l].Amplitude;
            }

            float Dist1 = FMath::Sqrt(MinDistSq);
            float Dist2 = FMath::Sqrt(SecondMinDistSq);
            if (Dist1 < 0.0001f) Dist1 = 0.0001f;
            if (Dist2 < 0.0001f) Dist2 = 0.0001f;

            float Weight1 = 1.0f / Dist1;
            float Weight2 = 1.0f / Dist2;
            float SumWeights = Weight1 + Weight2;

            FinalHeight = (Height1 * (Weight1 / SumWeights)) + (Height2 * (Weight2 / SumWeights));
        }
        else
        {
            FinalHeight = Height1;
        }
    }

    // 4. CRÁTERES
    if (Settings.bIsCraterPlanet)
    {
        float CellDistance = CraterNoise.GetNoise(X, Y, Z);
        float CraterRadius = Settings.CraterRadiusMultiplier;
        CellDistance = (CellDistance + 1.0f) * 0.5f;

        if (CellDistance < CraterRadius * 1.3f)
        {
            float t = CellDistance / CraterRadius;
            float craterHeight = 0.0f;

            if (t < 1.0f)
            {
                float floorStart = Settings.CraterFloorHeight;
                float bowl = (t < floorStart) ? 1.0f : FMath::Pow(1.0f - FMath::SmoothStep(0.0f, 1.0f, (t - floorStart) / (1.0f - floorStart)), 2.0f);
                craterHeight -= bowl * Settings.CraterDepth;
            }

            float rim = FMath::Exp(-FMath::Pow((t - 1.0f) / 0.15f, 2.0f) * Settings.CraterRimSharpness);
            craterHeight += rim * Settings.CraterDepth * Settings.CraterRimHeight;
            FinalHeight += craterHeight;
        }
    }

    // 5. SALIDA (Altura y Color)
    OutHeight = FinalHeight;

    float AltitudePenalty = FMath::Clamp(FinalHeight / MaxPossibleHeight, 0.0f, 1.0f);
    float VisualTemp = FMath::Clamp(FinalTemp - (AltitudePenalty * Settings.AltitudeTemperaturePenalty), 0.0f, 1.0f);
    float BiomeID = (ClosestBiomeIdx != -1) ? ((float)ClosestBiomeIdx / 255.0f) : 0.0f;

    OutColor = FLinearColor(AltitudePenalty, VisualTemp, FinalHum, BiomeID);
}