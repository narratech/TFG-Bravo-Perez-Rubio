// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicNoise.h"
#include "ThirdParty/FastNoiseLite.h"
#include "CosmicNoiseTypes.h"


TArray<float> CosmicNoise::CalculateHeights(const TArray<FVector>& Points, const FVector& PlanetCenter, const FTransform& ComponentTransform, FCosmicNoiseGenerationParameters Settings)
{
    TArray<float> OutHeights;

    // Comprobación de seguridad
    if (Points.IsEmpty())
    {
        return OutHeights;
    }

    const int32 PointCount = Points.Num();
    OutHeights.SetNumUninitialized(PointCount); // Reservar memoria exacta

    // Crear ruidos configurados una vez por capa
    TArray<TArray<FastNoiseLite>> BiomeNoises;
    BiomeNoises.SetNum(Settings.Biomes.Num());

    float MaxPossibleHeight = 0.0f;

    for (int i = 0; i < Settings.Biomes.Num(); i++)
    {
        float BiomeMaxHeight = 0.0f;
        const FCosmicBiomeData& BiomeData = Settings.Biomes[i];

        BiomeNoises[i].Reserve(BiomeData.NoiseLayers.Num());

        for (int j = 0; j < BiomeData.NoiseLayers.Num(); j++) {
            const FCosmicNoiseTypes& Layer = BiomeData.NoiseLayers[j];
            FastNoiseLite Noise;
            Noise.SetSeed(Settings.Seed + (i * 100) + j); //Semilla ligeramente distinta para no repetir

            // Noise Type 
            switch (Layer.NoiseType)
            {
            case ECosmicNoiseType::Perlin:
                Noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
                break;
            case ECosmicNoiseType::Simplex:
            case ECosmicNoiseType::Ridged: // Ridged usa Simplex como base
                Noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
                break;
            case ECosmicNoiseType::Cellular:
                Noise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
                break;
            case ECosmicNoiseType::Value:
                Noise.SetNoiseType(FastNoiseLite::NoiseType_Value);
                break;
            }

            // Fractal Type 
            switch (Layer.FractalType)
            {
            case ECosmicFractalType::None:
                Noise.SetFractalType(FastNoiseLite::FractalType_None);
                break;
            case ECosmicFractalType::FBM:
                Noise.SetFractalType(FastNoiseLite::FractalType_FBm);
                break;
            case ECosmicFractalType::Ridged:
                Noise.SetFractalType(FastNoiseLite::FractalType_Ridged);
                break;
            case ECosmicFractalType::PingPong:
                Noise.SetFractalType(FastNoiseLite::FractalType_PingPong);
                break;
            }

            // Parámetros fractales
            Noise.SetFrequency(Layer.Frequency);
            Noise.SetFractalOctaves(Layer.Octaves);
            Noise.SetFractalLacunarity(Layer.Lacunarity);
            Noise.SetFractalGain(Layer.Persistence);

            BiomeNoises[i].Add(Noise);
            BiomeMaxHeight += Layer.Amplitude;
        }
        MaxPossibleHeight = FMath::Max(MaxPossibleHeight, BiomeMaxHeight);
    }
    if (MaxPossibleHeight == 0.0f) MaxPossibleHeight = 1000.0f;

    FastNoiseLite HumidityNoise;
    HumidityNoise.SetSeed(Settings.Seed);
    HumidityNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    HumidityNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    HumidityNoise.SetFrequency(Settings.HumidityFrequency * 100.0f);
    HumidityNoise.SetFractalOctaves(Settings.HumidityOctaves);

    FastNoiseLite TempNoise;
    TempNoise.SetSeed(Settings.Seed);
    TempNoise.SetFrequency(Settings.TemperatureFrequency * 100.0f);

    FastNoiseLite CraterNoise;

    if (Settings.bIsCraterPlanet)
    {
        CraterNoise.SetSeed(Settings.Seed + 4242);
        CraterNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
        CraterNoise.SetFrequency(Settings.CraterFrequency);
        CraterNoise.SetFractalType(FastNoiseLite::FractalType_None);
        CraterNoise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean);
        CraterNoise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance);
    }

    // Domain Warp (configurado UNA VEZ) 
    FastNoiseLite WarpNoise;
    if (Settings.bUseDomainWarp)
    {
        WarpNoise.SetSeed(Settings.Seed + 1337);
        WarpNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        WarpNoise.SetFrequency(Settings.DomainWarpFrequency);
    }


    // Loop de puntos
    for (int32 i = 0; i < PointCount; i++)
    {
        // Asumimos que los puntos de entrada ya están transformados al espacio donde quieres evaluarlos.
        // Extraemos la dirección esférica basándonos en el centro del planeta.
        FVector WorldPos = ComponentTransform.TransformPosition(Points[i]);
        FVector NoiseDir = (WorldPos - PlanetCenter).GetSafeNormal();

        float X = NoiseDir.X;
        float Y = NoiseDir.Y;
        float Z = NoiseDir.Z;

        // Aplicar Domain Warp
        if (Settings.bUseDomainWarp)
        {
            float WarpX = WarpNoise.GetNoise(X, Y, Z) * Settings.DomainWarpStrength;
            float WarpY = WarpNoise.GetNoise(X + 31.7f, Y + 17.3f, Z + 47.1f) * Settings.DomainWarpStrength;
            float WarpZ = WarpNoise.GetNoise(X + 59.2f, Y + 11.8f, Z + 23.4f) * Settings.DomainWarpStrength;

            X += WarpX;
            Y += WarpY;
            Z += WarpZ;
        }

        float RawHum = HumidityNoise.GetNoise(NoiseDir.X, NoiseDir.Y, NoiseDir.Z);
        float FinalHum = (RawHum + 1.0f) * 0.5f + Settings.HumidityOffset;
        FinalHum = FMath::Clamp((FinalHum - 0.5f) * Settings.HumidityContrast + 0.5f, 0.0f, 1.0f);

        float Latitude = FMath::Abs(NoiseDir.Z); // 0 en el ecuador, 1 en los polos
        float BaseTemp = 1.0f - (Latitude * Settings.LatitudeEffect);
        float TempVariance = TempNoise.GetNoise(NoiseDir.X, NoiseDir.Y, NoiseDir.Z) * 0.2f;

        float FinalTemp = FMath::Clamp(BaseTemp + TempVariance, 0.0f, 1.0f);

        int32 ClosestBiomeIdx = -1;
        int32 SecondClosestBiomeIdx = -1;
        float MinDistSq = UE_BIG_NUMBER;
        float SecondMinDistSq = UE_BIG_NUMBER;

        for (int j = 0; j < Settings.Biomes.Num(); j++)
        {
            float dTemp = FinalTemp - Settings.Biomes[j].TargetTemperature;
            float dHum = FinalHum - Settings.Biomes[j].TargetHumidity;

            // Distancia al cuadrado (más rápido que hacer FMath::Sqrt)
            float DistSq = (dTemp * dTemp) + (dHum * dHum);

            if (DistSq < MinDistSq)
            {
                SecondMinDistSq = MinDistSq;
                SecondClosestBiomeIdx = ClosestBiomeIdx;
                MinDistSq = DistSq;
                ClosestBiomeIdx = j;
            }
            else if (DistSq < SecondMinDistSq)
            {
                SecondMinDistSq = DistSq;
                SecondClosestBiomeIdx = j;
            }
        }
        float FinalHeight = 0.0f;

        if (ClosestBiomeIdx != -1) {
            float Height1 = 0.0f;
            for (int k = 0; k < Settings.Biomes[ClosestBiomeIdx].NoiseLayers.Num(); k++) {
                Height1 += BiomeNoises[ClosestBiomeIdx][k].GetNoise(X, Y, Z) * Settings.Biomes[ClosestBiomeIdx].NoiseLayers[k].Amplitude;
            }

            if (SecondClosestBiomeIdx != -1)
            {
                // 2. Calcular altura del bioma secundario
                float Height2 = 0.0f;
                for (int32 l = 0; l < Settings.Biomes[SecondClosestBiomeIdx].NoiseLayers.Num(); l++) {
                    Height2 += BiomeNoises[SecondClosestBiomeIdx][l].GetNoise(X, Y, Z) * Settings.Biomes[SecondClosestBiomeIdx].NoiseLayers[l].Amplitude;
                }

                // 3. Calcular la fuerza (peso) de cada uno
                // Extraemos raíces cuadradas solo ahora al final para la matemática precisa del peso
                float Dist1 = FMath::Sqrt(MinDistSq);
                float Dist2 = FMath::Sqrt(SecondMinDistSq);

                // Evitar división por cero si estamos exactamente en el punto ideal
                if (Dist1 < 0.0001f) Dist1 = 0.0001f;
                if (Dist2 < 0.0001f) Dist2 = 0.0001f;

                float Weight1 = 1.0f / Dist1;
                float Weight2 = 1.0f / Dist2;
                float SumWeights = Weight1 + Weight2;

                Weight1 /= SumWeights;
                Weight2 /= SumWeights;

                // 4. Mezclar las dos alturas
                FinalHeight = (Height1 * Weight1) + (Height2 * Weight2);
            }
            else
            {
                // Si solo hay 1 bioma en todo el array
                FinalHeight = Height1;
            }
        }

        // Cráteres
        if (Settings.bIsCraterPlanet)
        {
            // Distancia al centro de la celda Voronoi
            float CellDistance = CraterNoise.GetNoise(X, Y, Z);
            float CraterRadius = Settings.CraterRadiusMultiplier;

            CellDistance = (CellDistance + 1.0f) * 0.5f;

            if (CellDistance < CraterRadius * 1.3f)
            {
                float t = CellDistance / CraterRadius; // 0 centro, 1 borde
                float craterHeight = 0.0f;

                // CAVIDAD 
                if (t < 1.0f)
                {
                    float floorStart = Settings.CraterFloorHeight; // 0.0 - 1.0
                    float bowl = 0.0f;

                    if (t < floorStart)
                    {
                        // zona plana del cráter
                        bowl = 1.0f;
                    }
                    else
                    {
                        // pared del cráter
                        float wallT = (t - floorStart) / (1.0f - floorStart);
                        wallT = FMath::SmoothStep(0.0f, 1.0f, wallT);
                        bowl = 1.0f - wallT;
                        bowl *= bowl;
                    }

                    craterHeight -= bowl * Settings.CraterDepth;
                }

                // RIM 
                float rimWidth = 0.15f;
                float rim = FMath::Exp(-FMath::Pow((t - 1.0f) / rimWidth, 2.0f) * Settings.CraterRimSharpness);

                craterHeight += rim * Settings.CraterDepth * Settings.CraterRimHeight;

                FinalHeight += craterHeight;
            }
        }

        // Asignamos directamente la altura al array
        OutHeights[i] = FinalHeight;
    }

    return OutHeights;
}

TArray<float> CosmicNoise::CalculateHeightsDirect(const TArray<FVector>& Points, FCosmicNoiseGenerationParameters Settings)
{
    TArray<float> OutHeights;

    // Comprobación de seguridad
    if (Points.IsEmpty())
    {
        return OutHeights;
    }

    const int32 PointCount = Points.Num();
    OutHeights.SetNumUninitialized(PointCount); // Reservar memoria exacta

    // Crear ruidos configurados una vez por capa
    TArray<TArray<FastNoiseLite>> BiomeNoises;
    BiomeNoises.SetNum(Settings.Biomes.Num());

    float MaxPossibleHeight = 0.0f;

    for (int i = 0; i < Settings.Biomes.Num(); i++)
    {
        float BiomeMaxHeight = 0.0f;
        const FCosmicBiomeData& BiomeData = Settings.Biomes[i];

        BiomeNoises[i].Reserve(BiomeData.NoiseLayers.Num());

        for (int j = 0; j < BiomeData.NoiseLayers.Num(); j++) {
            const FCosmicNoiseTypes& Layer = BiomeData.NoiseLayers[j];
            FastNoiseLite Noise;
            Noise.SetSeed(Settings.Seed + (i * 100) + j); //Semilla ligeramente distinta para no repetir

            // Noise Type 
            switch (Layer.NoiseType)
            {
            case ECosmicNoiseType::Perlin:
                Noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
                break;
            case ECosmicNoiseType::Simplex:
            case ECosmicNoiseType::Ridged: // Ridged usa Simplex como base
                Noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
                break;
            case ECosmicNoiseType::Cellular:
                Noise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
                break;
            case ECosmicNoiseType::Value:
                Noise.SetNoiseType(FastNoiseLite::NoiseType_Value);
                break;
            }

            // Fractal Type 
            switch (Layer.FractalType)
            {
            case ECosmicFractalType::None:
                Noise.SetFractalType(FastNoiseLite::FractalType_None);
                break;
            case ECosmicFractalType::FBM:
                Noise.SetFractalType(FastNoiseLite::FractalType_FBm);
                break;
            case ECosmicFractalType::Ridged:
                Noise.SetFractalType(FastNoiseLite::FractalType_Ridged);
                break;
            case ECosmicFractalType::PingPong:
                Noise.SetFractalType(FastNoiseLite::FractalType_PingPong);
                break;
            }

            // Parámetros fractales
            Noise.SetFrequency(Layer.Frequency);
            Noise.SetFractalOctaves(Layer.Octaves);
            Noise.SetFractalLacunarity(Layer.Lacunarity);
            Noise.SetFractalGain(Layer.Persistence);

            BiomeNoises[i].Add(Noise);
            BiomeMaxHeight += Layer.Amplitude;
        }
        MaxPossibleHeight = FMath::Max(MaxPossibleHeight, BiomeMaxHeight);
    }
    if (MaxPossibleHeight == 0.0f) MaxPossibleHeight = 1000.0f;

    FastNoiseLite HumidityNoise;
    HumidityNoise.SetSeed(Settings.Seed);
    HumidityNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    HumidityNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    HumidityNoise.SetFrequency(Settings.HumidityFrequency * 100.0f);
    HumidityNoise.SetFractalOctaves(Settings.HumidityOctaves);

    FastNoiseLite TempNoise;
    TempNoise.SetSeed(Settings.Seed);
    TempNoise.SetFrequency(Settings.TemperatureFrequency * 100.0f);

    FastNoiseLite CraterNoise;

    if (Settings.bIsCraterPlanet)
    {
        CraterNoise.SetSeed(Settings.Seed + 4242);
        CraterNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
        CraterNoise.SetFrequency(Settings.CraterFrequency);
        CraterNoise.SetFractalType(FastNoiseLite::FractalType_None);
        CraterNoise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean);
        CraterNoise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance);
    }

    // Domain Warp (configurado UNA VEZ) 
    FastNoiseLite WarpNoise;
    if (Settings.bUseDomainWarp)
    {
        WarpNoise.SetSeed(Settings.Seed + 1337);
        WarpNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        WarpNoise.SetFrequency(Settings.DomainWarpFrequency);
    }

    // Loop de puntos
    for (int32 i = 0; i < PointCount; i++)
    {
        float X = Points[i].X;
        float Y = Points[i].Y;
        float Z = Points[i].Z;

        // Aplicar Domain Warp
        if (Settings.bUseDomainWarp)
        {
            float WarpX = WarpNoise.GetNoise(X, Y, Z) * Settings.DomainWarpStrength;
            float WarpY = WarpNoise.GetNoise(X + 31.7f, Y + 17.3f, Z + 47.1f) * Settings.DomainWarpStrength;
            float WarpZ = WarpNoise.GetNoise(X + 59.2f, Y + 11.8f, Z + 23.4f) * Settings.DomainWarpStrength;

            X += WarpX;
            Y += WarpY;
            Z += WarpZ;
        }

        float RawHum = HumidityNoise.GetNoise(Points[i].X, Points[i].Y, Points[i].Z);
        float FinalHum = (RawHum + 1.0f) * 0.5f + Settings.HumidityOffset;
        FinalHum = FMath::Clamp((FinalHum - 0.5f) * Settings.HumidityContrast + 0.5f, 0.0f, 1.0f);

        float Latitude = FMath::Abs(Points[i].Z); // 0 en el ecuador, 1 en los polos
        float BaseTemp = 1.0f - (Latitude * Settings.LatitudeEffect);
        float TempVariance = TempNoise.GetNoise(Points[i].X, Points[i].Y, Points[i].Z) * 0.2f;

        float FinalTemp = FMath::Clamp(BaseTemp + TempVariance, 0.0f, 1.0f);

        int32 ClosestBiomeIdx = -1;
        int32 SecondClosestBiomeIdx = -1;
        float MinDistSq = UE_BIG_NUMBER;
        float SecondMinDistSq = UE_BIG_NUMBER;

        for (int j = 0; j < Settings.Biomes.Num(); j++)
        {
            float dTemp = FinalTemp - Settings.Biomes[j].TargetTemperature;
            float dHum = FinalHum - Settings.Biomes[j].TargetHumidity;

            // Distancia al cuadrado (más rápido que hacer FMath::Sqrt)
            float DistSq = (dTemp * dTemp) + (dHum * dHum);

            if (DistSq < MinDistSq)
            {
                SecondMinDistSq = MinDistSq;
                SecondClosestBiomeIdx = ClosestBiomeIdx;
                MinDistSq = DistSq;
                ClosestBiomeIdx = j;
            }
            else if (DistSq < SecondMinDistSq)
            {
                SecondMinDistSq = DistSq;
                SecondClosestBiomeIdx = j;
            }
        }
        float FinalHeight = 0.0f;

        if (ClosestBiomeIdx != -1) {
            float Height1 = 0.0f;
            for (int k = 0; k < Settings.Biomes[ClosestBiomeIdx].NoiseLayers.Num(); k++) {
                Height1 += BiomeNoises[ClosestBiomeIdx][k].GetNoise(X, Y, Z) * Settings.Biomes[ClosestBiomeIdx].NoiseLayers[k].Amplitude;
            }

            if (SecondClosestBiomeIdx != -1)
            {
                // 2. Calcular altura del bioma secundario
                float Height2 = 0.0f;
                for (int32 l = 0; l < Settings.Biomes[SecondClosestBiomeIdx].NoiseLayers.Num(); l++) {
                    Height2 += BiomeNoises[SecondClosestBiomeIdx][l].GetNoise(X, Y, Z) * Settings.Biomes[SecondClosestBiomeIdx].NoiseLayers[l].Amplitude;
                }

                // 3. Calcular la fuerza (peso) de cada uno
                // Extraemos raíces cuadradas solo ahora al final para la matemática precisa del peso
                float Dist1 = FMath::Sqrt(MinDistSq);
                float Dist2 = FMath::Sqrt(SecondMinDistSq);

                // Evitar división por cero si estamos exactamente en el punto ideal
                if (Dist1 < 0.0001f) Dist1 = 0.0001f;
                if (Dist2 < 0.0001f) Dist2 = 0.0001f;

                float Weight1 = 1.0f / Dist1;
                float Weight2 = 1.0f / Dist2;
                float SumWeights = Weight1 + Weight2;

                Weight1 /= SumWeights;
                Weight2 /= SumWeights;

                // 4. Mezclar las dos alturas
                FinalHeight = (Height1 * Weight1) + (Height2 * Weight2);
            }
            else
            {
                // Si solo hay 1 bioma en todo el array
                FinalHeight = Height1;
            }
        }

        // Cráteres
        if (Settings.bIsCraterPlanet)
        {
            // Distancia al centro de la celda Voronoi
            float CellDistance = CraterNoise.GetNoise(X, Y, Z);
            float CraterRadius = Settings.CraterRadiusMultiplier;

            CellDistance = (CellDistance + 1.0f) * 0.5f;

            if (CellDistance < CraterRadius * 1.3f)
            {
                float t = CellDistance / CraterRadius; // 0 centro, 1 borde
                float craterHeight = 0.0f;

                // CAVIDAD 
                if (t < 1.0f)
                {
                    float floorStart = Settings.CraterFloorHeight; // 0.0 - 1.0
                    float bowl = 0.0f;

                    if (t < floorStart)
                    {
                        // zona plana del cráter
                        bowl = 1.0f;
                    }
                    else
                    {
                        // pared del cráter
                        float wallT = (t - floorStart) / (1.0f - floorStart);
                        wallT = FMath::SmoothStep(0.0f, 1.0f, wallT);
                        bowl = 1.0f - wallT;
                        bowl *= bowl;
                    }

                    craterHeight -= bowl * Settings.CraterDepth;
                }

                // RIM 
                float rimWidth = 0.15f;
                float rim = FMath::Exp(-FMath::Pow((t - 1.0f) / rimWidth, 2.0f) * Settings.CraterRimSharpness);

                craterHeight += rim * Settings.CraterDepth * Settings.CraterRimHeight;

                FinalHeight += craterHeight;
            }
        }

        // Asignamos directamente la altura al array
        OutHeights[i] = FinalHeight;
    }

    return OutHeights;
}

