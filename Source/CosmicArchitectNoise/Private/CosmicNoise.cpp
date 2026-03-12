// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicNoise.h"
#include "CosmicNoiseSettings.h"
#include "ThirdParty/FastNoiseLite.h"
#include "CosmicNoiseTypes.h"


TArray<float> CosmicNoise::CalculateHeights(const TArray<FVector>& Points, const FVector& PlanetCenter, const FTransform& ComponentTransform, const UCosmicNoiseSettings* Settings)
{
    TArray<float> OutHeights;

    // Comprobación de seguridad
    if (!Settings || Points.IsEmpty())
    {
        return OutHeights;
    }

    const int32 PointCount = Points.Num();
    OutHeights.SetNumUninitialized(PointCount); // Reservar memoria exacta

    // Crear ruidos configurados una vez por capa
    TArray<FastNoiseLite> ConfiguredNoises;
    ConfiguredNoises.Reserve(Settings->NoiseLayers.Num());

    FastNoiseLite CraterNoise;

    if (Settings->bIsCraterPlanet)
    {
        CraterNoise.SetSeed(Settings->Seed + 4242);
        CraterNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
        CraterNoise.SetFrequency(Settings->CraterFrequency);
        CraterNoise.SetFractalType(FastNoiseLite::FractalType_None);
        CraterNoise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean);
        CraterNoise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance);
    }

    for (const FCosmicNoiseTypes& Layer : Settings->NoiseLayers)
    {
        FastNoiseLite Noise;
        Noise.SetSeed(Settings->Seed);

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
    if (Settings->bUseDomainWarp)
    {
        WarpNoise.SetSeed(Settings->Seed + 1337);
        WarpNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        WarpNoise.SetFrequency(Settings->DomainWarpFrequency);
    }

    const bool bHasLayers = Settings->NoiseLayers.Num() > 0;

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
        if (Settings->bUseDomainWarp)
        {
            float WarpX = WarpNoise.GetNoise(X, Y, Z) * Settings->DomainWarpStrength;
            float WarpY = WarpNoise.GetNoise(X + 31.7f, Y + 17.3f, Z + 47.1f) * Settings->DomainWarpStrength;
            float WarpZ = WarpNoise.GetNoise(X + 59.2f, Y + 11.8f, Z + 23.4f) * Settings->DomainWarpStrength;

            X += WarpX;
            Y += WarpY;
            Z += WarpZ;
        }

        float BaseHeight = 0.0f;

        // Altura base del terreno (capas de ruido)
        for (int32 LayerIndex = 0; LayerIndex < Settings->NoiseLayers.Num(); LayerIndex++)
        {
            const FCosmicNoiseTypes& Layer = Settings->NoiseLayers[LayerIndex];
            FastNoiseLite& Noise = ConfiguredNoises[LayerIndex];

            float LayerNoise = Noise.GetNoise(X, Y, Z);
            BaseHeight += LayerNoise * Layer.Amplitude;
        }

        // Fallback si no hay capas
        if (!bHasLayers)
        {
            FastNoiseLite DefaultNoise;
            DefaultNoise.SetSeed(Settings->Seed);
            DefaultNoise.SetFrequency(0.001f);
            BaseHeight = DefaultNoise.GetNoise(X, Y, Z) * 1000.0f;
        }

        float FinalHeight = BaseHeight;

        // Cráteres
        if (Settings->bIsCraterPlanet)
        {
            // Distancia al centro de la celda Voronoi
            float CellDistance = CraterNoise.GetNoise(X, Y, Z);
            float CraterRadius = Settings->CraterRadiusMultiplier;

            CellDistance = (CellDistance + 1.0f) * 0.5f;

            if (CellDistance < CraterRadius * 1.3f)
            {
                float t = CellDistance / CraterRadius; // 0 centro, 1 borde
                float craterHeight = 0.0f;

                // CAVIDAD 
                if (t < 1.0f)
                {
                    float floorStart = Settings->CraterFloorHeight; // 0.0 - 1.0
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

                    craterHeight -= bowl * Settings->CraterDepth;
                }

                // RIM 
                float rimWidth = 0.15f;
                float rim = FMath::Exp(-FMath::Pow((t - 1.0f) / rimWidth, 2.0f) * Settings->CraterRimSharpness);

                craterHeight += rim * Settings->CraterDepth * Settings->CraterRimHeight;

                FinalHeight += craterHeight;
            }
        }

        // Asignamos directamente la altura al array
        OutHeights[i] = FinalHeight;
    }

    

    return OutHeights;
}

