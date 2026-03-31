#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"
#include "ThirdParty/FastNoiseLite.h"
#include "CosmicNoiseTypes.h"
#include "CosmicNoiseSettings.h"

class FCosmicArchitectNoiseGenerator: public FNonAbandonableTask {
public:
	// Referencias a los datos inmutables de la malla
	const TArray<FVector>& BaseVertices;
	const TArray<FVector>& BaseNormals;

	// El array donde guardaremos el resultado
	TArray<FVector> CalculatedVertices;
    TArray<FLinearColor> CalculatedColors;

	// Datos de transformación
	FTransform ComponentTransform;
	FVector PlanetCenter;

    FCosmicNoiseGenerationParameters NoiseSettings;

    FCosmicArchitectNoiseGenerator(
        const TArray<FVector>& InBaseVerts,
        const TArray<FVector>& InBaseNormals,
        FTransform InTransform,
        FVector InPlanetCenter,
        FCosmicNoiseGenerationParameters NoiseSettings)
        : BaseVertices(InBaseVerts)
        , BaseNormals(InBaseNormals)
        , ComponentTransform(InTransform)
        , PlanetCenter(InPlanetCenter),
        NoiseSettings(NoiseSettings)
    {
        CalculatedVertices.SetNumUninitialized(BaseVertices.Num());
        CalculatedColors.SetNumUninitialized(BaseVertices.Num());
    }

    

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FCosmicArchitectNoiseGenerator, STATGROUP_ThreadPoolAsyncTasks);
	}

    void DoWork()
    {

        // Crear ruidos configurados una vez por capa
        TArray<TArray<FastNoiseLite>> BiomeNoises;
        BiomeNoises.SetNum(NoiseSettings.Biomes.Num());

        float MaxPossibleHeight = 0.0f;

        for (int i = 0; i < NoiseSettings.Biomes.Num(); i++)
        {
            float BiomeMaxHeight = 0.0f;
            const FCosmicBiomeData& BiomeData = NoiseSettings.Biomes[i];

            BiomeNoises[i].Reserve(BiomeData.NoiseLayers.Num());

            for (int j = 0; j < BiomeData.NoiseLayers.Num(); j++) {
                const FCosmicNoiseTypes& Layer = BiomeData.NoiseLayers[j];
                FastNoiseLite Noise;
                Noise.SetSeed(NoiseSettings.Seed + (i * 100) + j); //Semilla ligeramente distinta para no repetir

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

        //Temperatura y humedad
        FastNoiseLite HumidityNoise;
        HumidityNoise.SetSeed(NoiseSettings.Seed);
        HumidityNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        HumidityNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
        HumidityNoise.SetFrequency(NoiseSettings.HumidityFrequency * 100.0f);
        HumidityNoise.SetFractalOctaves(NoiseSettings.HumidityOctaves);

        FastNoiseLite TempNoise;
        TempNoise.SetSeed(NoiseSettings.Seed);
        TempNoise.SetFrequency(NoiseSettings.TemperatureFrequency * 100.0f);

        FastNoiseLite CraterNoise;

        if (NoiseSettings.bIsCraterPlanet)
        {
            CraterNoise.SetSeed(NoiseSettings.Seed + 4242);
            CraterNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
            CraterNoise.SetFrequency(NoiseSettings.CraterFrequency);
            CraterNoise.SetFractalType(FastNoiseLite::FractalType_None);
            //CraterNoise.SetFractalOctaves(CraterOctaves);
            //CraterNoise.SetFractalLacunarity(CraterLacunarity);
            //CraterNoise.SetFractalGain(CraterPersistence);
            CraterNoise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean);
            CraterNoise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance);

        }

        // Domain Warp (configurado UNA VEZ) 
        FastNoiseLite WarpNoise;
        if (NoiseSettings.bUseDomainWarp)
        {
            WarpNoise.SetSeed(NoiseSettings.Seed + 1337);
            WarpNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
            WarpNoise.SetFrequency(NoiseSettings.DomainWarpFrequency);
        }

        const int32 VertexCount = BaseVertices.Num();

        // Loop de vértices
        for (int32 i = 0; i < VertexCount; i++)
        {
            FVector WorldPos = ComponentTransform.TransformPosition(BaseVertices[i]);
            FVector NoiseDir = (WorldPos - PlanetCenter).GetSafeNormal();

            float X = NoiseDir.X;
            float Y = NoiseDir.Y;
            float Z = NoiseDir.Z;

            // Aplicar Domain Warp
            if (NoiseSettings.bUseDomainWarp)
            {
                float WarpX = WarpNoise.GetNoise(X, Y, Z) * NoiseSettings.DomainWarpStrength;
                float WarpY = WarpNoise.GetNoise(X + 31.7f, Y + 17.3f, Z + 47.1f) * NoiseSettings.DomainWarpStrength;
                float WarpZ = WarpNoise.GetNoise(X + 59.2f, Y + 11.8f, Z + 23.4f) * NoiseSettings.DomainWarpStrength;

                X += WarpX;
                Y += WarpY;
                Z += WarpZ;
            }

            float RawHum = HumidityNoise.GetNoise(NoiseDir.X, NoiseDir.Y, NoiseDir.Z);
            float BaseHum = (RawHum + 1.0f) * 0.5f + NoiseSettings.HumidityOffset;
            float FinalHum = FMath::Clamp((BaseHum - 0.5f) * NoiseSettings.HumidityContrast + 0.5f, 0.0f, 1.0f);
           
            float Latitude = FMath::Abs(NoiseDir.Z); // 0 en el ecuador, 1 en los polos
            float BaseTemp = 1.0f - (Latitude * NoiseSettings.LatitudeEffect);
            float TempVariance = TempNoise.GetNoise(NoiseDir.X, NoiseDir.Y, NoiseDir.Z) * 0.2f;

            float FinalTemp = FMath::Clamp(BaseTemp + TempVariance, 0.0f, 1.0f);

            int32 ClosestBiomeIdx = -1;
            int32 SecondClosestBiomeIdx = -1;
            float MinDistSq = UE_BIG_NUMBER;
            float SecondMinDistSq = UE_BIG_NUMBER;

            for (int j = 0; j < NoiseSettings.Biomes.Num(); j++)
            {
                float dTemp = FinalTemp - NoiseSettings.Biomes[j].TargetTemperature;
                float dHum = FinalHum - NoiseSettings.Biomes[j].TargetHumidity;

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
                for (int k = 0; k < NoiseSettings.Biomes[ClosestBiomeIdx].NoiseLayers.Num(); k++) {
                    Height1 += BiomeNoises[ClosestBiomeIdx][k].GetNoise(X, Y, Z) * NoiseSettings.Biomes[ClosestBiomeIdx].NoiseLayers[k].Amplitude;
                }

                if (SecondClosestBiomeIdx != -1)
                {
                    // 2. Calcular altura del bioma secundario
                    float Height2 = 0.0f;
                    for (int32 l = 0; l < NoiseSettings.Biomes[SecondClosestBiomeIdx].NoiseLayers.Num(); l++) {
                        Height2 += BiomeNoises[SecondClosestBiomeIdx][l].GetNoise(X, Y, Z) * NoiseSettings.Biomes[SecondClosestBiomeIdx].NoiseLayers[l].Amplitude;
                    }

                    // 3. Calcular la fuerza (peso) de cada uno
                    // Extraemos raíces cuadradas solo ahora al final para la matemática precisa del peso
                    float Dist1 = FMath::Sqrt(MinDistSq);
                    float Dist2 = FMath::Sqrt(SecondMinDistSq);

                    // Evitar división por cero si estamos exactamente en el punto ideal
                    if (Dist1 < 0.0001f) Dist1 = 0.0001f;
                    if (Dist2 < 0.0001f) Dist2 = 0.0001f;

                    float BlendSharpness = 5.0f;

                    float Weight1 = 1.0f / FMath::Pow(Dist1, BlendSharpness);
                    float Weight2 = 1.0f / FMath::Pow(Dist2, BlendSharpness);
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

            if (NoiseSettings.bIsCraterPlanet)
            {
                // Distancia al centro de la celda Voronoi
                float CellDistance = CraterNoise.GetNoise(X, Y, Z);
                float CraterRadius = NoiseSettings.CraterRadiusMultiplier;

                CellDistance = (CellDistance + 1.0f) * 0.5f;

                if (CellDistance < CraterRadius * 1.3f)
                {
                    float t = CellDistance / CraterRadius;// 0 centro, 1 borde

                    float craterHeight = 0.0f;

                    // CAVIDAD 
                    if (t < 1.0f)
                    {
                        float floorStart = NoiseSettings.CraterFloorHeight; // 0.0 - 1.0

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

                        craterHeight -= bowl * NoiseSettings.CraterDepth;
                    }

                    // RIM 
                    float rimWidth = 0.15f;

                    float rim = FMath::Exp(-FMath::Pow((t - 1.0f) / rimWidth, 2.0f) * NoiseSettings.CraterRimSharpness);

                    craterHeight += rim * NoiseSettings.CraterDepth * NoiseSettings.CraterRimHeight;

                    FinalHeight += craterHeight;
                }
            }

            // Calcular posición final del vértice
            CalculatedVertices[i] = BaseVertices[i] + (BaseNormals[i] * FinalHeight);

            // Penalización por altitud (usando FinalHeight)
            float AltitudePenalty = FMath::Clamp(FinalHeight / MaxPossibleHeight, 0.0f, 1.0f);

            float VisualTemp = FMath::Clamp(FinalTemp - (AltitudePenalty * NoiseSettings.AltitudeTemperaturePenalty), 0.0f, 1.0f);
            float BiomeID = (ClosestBiomeIdx != -1) ? ((float)ClosestBiomeIdx / 255.0f) : 0.0f;
            // Guardar colores
            CalculatedColors[i] = FLinearColor(AltitudePenalty, VisualTemp, FinalHum, BiomeID);
            
        }
    }
};