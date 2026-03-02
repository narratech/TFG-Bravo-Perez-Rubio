#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"
#include "ThirdParty/FastNoiseLite.h"
#include "CosmicNoiseTypes.h"

class FCosmicArchitectNoiseGenerator: public FNonAbandonableTask {
public:
	// Referencias a los datos inmutables de la malla
	const TArray<FVector>& BaseVertices;
	const TArray<FVector>& BaseNormals;

	// El array donde guardaremos el resultado
	TArray<FVector> CalculatedVertices;

	// Datos de transformación
	FTransform ComponentTransform;
	FVector PlanetCenter;

	int32 Seed;
	TArray<FCosmicNoiseTypes> Layers;
	bool bUseDomainWarp;
	float DomainWarpStrength;
	float DomainWarpFrequency;

	FCosmicArchitectNoiseGenerator(
		const TArray<FVector>& InBaseVerts,
		const TArray<FVector>& InBaseNormals,
		FTransform InTransform,
		FVector InPlanetCenter,
		int32 InSeed,
		const TArray<FCosmicNoiseTypes>& InLayers,
		bool InUseWarp,
		float InWarpStrength,
		float InWarpFreq)
		: BaseVertices(InBaseVerts), BaseNormals(InBaseNormals),
		ComponentTransform(InTransform), PlanetCenter(InPlanetCenter),
		Seed(InSeed), Layers(InLayers),
		bUseDomainWarp(InUseWarp), DomainWarpStrength(InWarpStrength), DomainWarpFrequency(InWarpFreq)
	{
		CalculatedVertices.SetNumUninitialized(BaseVertices.Num());
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FCosmicArchitectNoiseGenerator, STATGROUP_ThreadPoolAsyncTasks);
	}

    void DoWork()
    {
        // Configurar ruido con seed
        FastNoiseLite Noise;
        Noise.SetSeed(Seed);
        Noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

        // Si hay capas, usar la primera como principal
        if (Layers.Num() > 0)
        {
            const FCosmicNoiseTypes& MainLayer = Layers[0];
            // Configurar Noise según MainLayer
            // Nota: FastNoiseLite tiene API limitada, necesitarías múltiples instancias
            // o un sistema más complejo. Por ahora usamos valores simples.
        }

        for (int32 i = 0; i < BaseVertices.Num(); i++)
        {
            FVector WorldPos = ComponentTransform.TransformPosition(BaseVertices[i]);
            FVector NoiseDir = (WorldPos - PlanetCenter).GetSafeNormal();

            float TotalNoise = 0.0f;

            // Sumar todas las capas de ruido
            for (const FCosmicNoiseTypes& Layer : Layers)
            {
                float LayerNoise = 0.0f;

                // Configurar tipo de ruido según Layer
                switch (Layer.NoiseType)
                {
                case ECosmicNoiseType::Perlin:
                    Noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
                    break;
                case ECosmicNoiseType::Simplex:
                    Noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
                    break;
                case ECosmicNoiseType::Cellular:
                    Noise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
                    break;
                case ECosmicNoiseType::Ridged:
                    Noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
                    Noise.SetFractalType(FastNoiseLite::FractalType_Ridged);
                    break;
                }

                LayerNoise = Noise.GetNoise(
                    NoiseDir.X * Layer.Frequency,
                    NoiseDir.Y * Layer.Frequency,
                    NoiseDir.Z * Layer.Frequency
                );

                // Aplicar amplitud de la capa
                TotalNoise += LayerNoise * Layer.Amplitude;
            }

            // Si no hay capas, usar ruido simple
            if (Layers.Num() == 0)
            {
                TotalNoise = Noise.GetNoise(
                    NoiseDir.X * 50000,
                    NoiseDir.Y * 50000,
                    NoiseDir.Z * 50000
                ) * 3000;
            }

            CalculatedVertices[i] = BaseVertices[i] + (BaseNormals[i] * TotalNoise);
        }
    }
};