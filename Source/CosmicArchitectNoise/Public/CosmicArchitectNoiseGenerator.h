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
        TArray<FastNoiseLite> ConfiguredNoisesA;
        ConfiguredNoisesA.Reserve(NoiseSettings.NoiseLayersA.Num());
        TArray<FastNoiseLite> ConfiguredNoisesB;
        ConfiguredNoisesB.Reserve(NoiseSettings.NoiseLayersB.Num());

        //Temperatura y humedad
        FastNoiseLite HumidityNoise;
        FastNoiseLite TempNoise;
        FastNoiseLite CraterNoise;
        FastNoiseLite CraterSizeNoise;

        HumidityNoise.SetSeed(NoiseSettings.Seed);
        HumidityNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        HumidityNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
        HumidityNoise.SetFrequency(NoiseSettings.HumidityFrequency * 100.0f);
        HumidityNoise.SetFractalOctaves(NoiseSettings.HumidityOctaves);

        TempNoise.SetSeed(NoiseSettings.Seed);
        TempNoise.SetFrequency(NoiseSettings.TemperatureFrequency * 100.0f);

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

        float MaxPossibleHeightA = 0.0f;
        for (const FCosmicNoiseTypes& Layer : NoiseSettings.NoiseLayersA)
        {
            FastNoiseLite Noise;
            Noise.SetSeed(NoiseSettings.Seed);

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

            ConfiguredNoisesA.Add(Noise);
            MaxPossibleHeightA += Layer.Amplitude;
        }
        float MaxPossibleHeightB = 0.0f;
        for (const FCosmicNoiseTypes& Layer : NoiseSettings.NoiseLayersB)
        {
            FastNoiseLite Noise;
            Noise.SetSeed(NoiseSettings.Seed);

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

            ConfiguredNoisesB.Add(Noise);
            MaxPossibleHeightB += Layer.Amplitude;
        }

        // Domain Warp (configurado UNA VEZ) 
        FastNoiseLite WarpNoise;
        if (NoiseSettings.bUseDomainWarp)
        {
            WarpNoise.SetSeed(NoiseSettings.Seed + 1337);
            WarpNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
            WarpNoise.SetFrequency(NoiseSettings.DomainWarpFrequency);
        }

        float MaxPossibleHeight = FMath::Max(MaxPossibleHeightA, MaxPossibleHeightB);
        if (MaxPossibleHeight == 0.0f) MaxPossibleHeight = 1000.0f; // Seguridad

        const bool bHasLayersA = NoiseSettings.NoiseLayersA.Num() > 0;
        const bool bHasLayersB = NoiseSettings.NoiseLayersB.Num() > 0;

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
            BaseHum = FMath::Clamp((BaseHum - 0.5f) * NoiseSettings.HumidityContrast + 0.5f, 0.0f, 1.0f);
            float BiomeMask = FMath::SmoothStep(0.3f, 0.7f, BaseHum);// Usamos un SmoothStep para que la transición entre biomas no sea tan lineal

            float HeightA = 0.0f; // Llanuras
            float HeightB = 0.0f; // Montañas

            // Solo calculamos A si la máscara no es 100% montaña
            if (BiomeMask < 1.0f)
            {
                for (int32 j = 0; j < NoiseSettings.NoiseLayersA.Num(); j++) {
                    HeightA += ConfiguredNoisesA[j].GetNoise(X, Y, Z) * NoiseSettings.NoiseLayersA[j].Amplitude;
                }
            }

            // Solo calculamos B si la máscara no es 100% llanura
            if (BiomeMask > 0.0f)
            {
                for (int32 j = 0; j < NoiseSettings.NoiseLayersB.Num(); j++) {
                    HeightB += ConfiguredNoisesB[j].GetNoise(X, Y, Z) * NoiseSettings.NoiseLayersB[j].Amplitude;
                }
            }
            if (!bHasLayersA)HeightA = 0.0f;
            if (!bHasLayersB)HeightB = 0.0f;

            float BaseHeight = FMath::Lerp(HeightA, HeightB, BiomeMask);
            float FinalHeight = BaseHeight;

           
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

            // --- CÁLCULO DE COLOR (Temperatura y Humedad) ---
            float Latitude = FMath::Abs(NoiseDir.Z);
            float BaseTemp = 1.0f - (Latitude * NoiseSettings.LatitudeEffect);

            // Penalización por altitud (usando FinalHeight)
            float AltitudePenalty = FMath::Clamp(FinalHeight / MaxPossibleHeight, 0.0f, 1.0f);
            BaseTemp -= (AltitudePenalty * NoiseSettings.AltitudeTemperaturePenalty);

            // Ruido térmico
            float TempVariance = TempNoise.GetNoise(NoiseDir.X, NoiseDir.Y, NoiseDir.Z) * 0.2f;
            float FinalTemp = FMath::Clamp(BaseTemp + TempVariance, 0.0f, 1.0f);

            // Guardar colores
            CalculatedColors[i] = FLinearColor(AltitudePenalty, FinalTemp, BiomeMask, 1.0f);
            
        }
    }
};