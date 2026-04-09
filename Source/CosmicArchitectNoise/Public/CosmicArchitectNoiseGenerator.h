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

	// El array donde guardaremos el resultado
	TArray<FVector> CalculatedVertices;
    TArray<FVector> CalculatedNormals;
    TArray<FLinearColor> CalculatedColors;

	// Datos de transformación
	FTransform ComponentTransform;
	FVector PlanetCenter;
    bool IsPlanet;
    FCosmicNoiseGenerationParameters NoiseSettings;

    FCosmicArchitectNoiseGenerator(
        const TArray<FVector>& InBaseVerts,
        FTransform InTransform,
        FVector InPlanetCenter,
        bool InPlanet,
        FCosmicNoiseGenerationParameters NoiseSettings)
        : BaseVertices(InBaseVerts)
        , ComponentTransform(InTransform)
        , PlanetCenter(InPlanetCenter)
        ,IsPlanet(InPlanet),
        NoiseSettings(NoiseSettings)
    {
        CalculatedVertices.SetNumUninitialized(BaseVertices.Num());
        CalculatedColors.SetNumUninitialized(BaseVertices.Num());
        CalculatedNormals.SetNumUninitialized(BaseVertices.Num());
    }

    

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FCosmicArchitectNoiseGenerator, STATGROUP_ThreadPoolAsyncTasks);
	}

    void DoWork()
    {

        const int32 VertexCount = BaseVertices.Num();

        if (IsPlanet) {
            
            FMatrix TransformMatrix = ComponentTransform.ToMatrixWithScale();

            for (size_t i = 0; i < VertexCount; i++)
            {
                CalculatedVertices[i] = TransformMatrix.TransformPosition(BaseVertices[i] + PlanetCenter);
            }
        }
        
        // Crear ruidos configurados una vez por capa
        TArray<FastNoiseLite> ConfiguredNoises;
        ConfiguredNoises.Reserve(NoiseSettings.NoiseLayers.Num());

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

        for (const FCosmicNoiseTypes& Layer : NoiseSettings.NoiseLayers)
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

            ConfiguredNoises.Add(Noise);
        }

        // Domain Warp (configurado UNA VEZ) 
        FastNoiseLite WarpNoise;
        if (NoiseSettings.bUseDomainWarp)
        {
            WarpNoise.SetSeed(NoiseSettings.Seed + 1337);
            WarpNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
            WarpNoise.SetFrequency(NoiseSettings.DomainWarpFrequency);
        }

        // Calcular la altura máxima posible sumando las amplitudes de las capas
        float MaxPossibleHeight = 0.0f;
        for (const FCosmicNoiseTypes& Layer : NoiseSettings.NoiseLayers)
        {
            MaxPossibleHeight += Layer.Amplitude;
        }
        if (MaxPossibleHeight == 0.0f) MaxPossibleHeight = 1000.0f; // Seguridad

        
        const bool bHasLayers = NoiseSettings.NoiseLayers.Num() > 0;

        // Loop de vértices
        for (int32 i = 0; i < VertexCount; i++)
        {
            FVector WorldPos = IsPlanet ? CalculatedVertices[i] : BaseVertices[i];
            FVector NoiseDir = IsPlanet ? (WorldPos - PlanetCenter).GetSafeNormal() : FVector(WorldPos.X, WorldPos.Y, 0);

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

            float BaseHeight = 0.0f;

            // Altura base del terreno (capas de ruido)
            for (int32 LayerIndex = 0; LayerIndex < NoiseSettings.NoiseLayers.Num(); LayerIndex++)
            {
                const FCosmicNoiseTypes& Layer = NoiseSettings.NoiseLayers[LayerIndex];
                FastNoiseLite& Noise = ConfiguredNoises[LayerIndex];

                float LayerNoise = Noise.GetNoise(X, Y, Z);
                BaseHeight += LayerNoise * Layer.Amplitude;
            }

            // Fallback si no hay capas
            if (!bHasLayers)
            {
                FastNoiseLite DefaultNoise;
                DefaultNoise.SetSeed(NoiseSettings.Seed);
                DefaultNoise.SetFrequency(0.001f);
                BaseHeight = DefaultNoise.GetNoise(X, Y, Z) * 1000.0f;
            }

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
            if (IsPlanet) {
                CalculatedNormals[i] = NoiseDir;
                CalculatedVertices[i] += (NoiseDir * FinalHeight);
            }
            else {
                CalculatedVertices[i] = BaseVertices[i] + (FVector::UpVector * FinalHeight);
            }
            

            // --- CÁLCULO DE COLOR (Temperatura y Humedad) ---
            float Latitude = FMath::Abs(NoiseDir.Z);
            float BaseTemp = 1.0f - (Latitude * NoiseSettings.LatitudeEffect);

            // Penalización por altitud (usando FinalHeight)
            float AltitudePenalty = FMath::Clamp(FinalHeight / MaxPossibleHeight, 0.0f, 1.0f);
            BaseTemp -= (AltitudePenalty * NoiseSettings.AltitudeTemperaturePenalty);

            // Ruido térmico
            float TempVariance = TempNoise.GetNoise(NoiseDir.X, NoiseDir.Y, NoiseDir.Z) * 0.2f;
            float FinalTemp = FMath::Clamp(BaseTemp + TempVariance, 0.0f, 1.0f);

            // Humedad 
            float RawHum = HumidityNoise.GetNoise(NoiseDir.X, NoiseDir.Y, NoiseDir.Z);
            float FinalHum = (RawHum + 1.0f) * 0.5f + NoiseSettings.HumidityOffset;
            FinalHum = FMath::Clamp((FinalHum - 0.5f) * NoiseSettings.HumidityContrast + 0.5f, 0.0f, 1.0f);

            // Guardar colores
            CalculatedColors[i] = FLinearColor(AltitudePenalty, FinalTemp, FinalHum, 1.0f);
            
        }
    }
};