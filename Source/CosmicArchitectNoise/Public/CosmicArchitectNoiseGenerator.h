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
    TArray<FLinearColor> CalculatedColors;

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
        CalculatedColors.SetNumUninitialized(BaseVertices.Num());
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FCosmicArchitectNoiseGenerator, STATGROUP_ThreadPoolAsyncTasks);
	}

    void DoWork()
    {
        // Crear ruidos configurados una vez por capa
        TArray<FastNoiseLite> ConfiguredNoises;
        ConfiguredNoises.Reserve(Layers.Num());

        //Temperatura y humedad
        FastNoiseLite HumidityNoise;
        FastNoiseLite TempNoise;

        HumidityNoise.SetSeed(33233);
        HumidityNoise.SetFrequency(0.002f);
        HumidityNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        HumidityNoise.SetFractalType(FastNoiseLite::FractalType_FBm); // Activar modo fractal
        HumidityNoise.SetFractalOctaves(5); // Añadir 5 capas de detalle (como nubes pequeñas sobre nubes grandes)
        HumidityNoise.SetFrequency(0.015f);

        TempNoise.SetSeed(33422);
        TempNoise.SetFrequency(0.005f);

        for (const FCosmicNoiseTypes& Layer : Layers)
        {
            FastNoiseLite Noise;
            Noise.SetSeed(Seed);

            // Noise Type 
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

            case ECosmicNoiseType::Value:
                Noise.SetNoiseType(FastNoiseLite::NoiseType_Value);
                break;

            case ECosmicNoiseType::Ridged:
                Noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
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
        if (bUseDomainWarp)
        {
            WarpNoise.SetSeed(Seed + 1337);
            WarpNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
            WarpNoise.SetFrequency(DomainWarpFrequency);
        }

        // Calcular la altura máxima posible sumando las amplitudes de las capas
        float MaxPossibleHeight = 0.0f;
        for (const FCosmicNoiseTypes& Layer : Layers)
        {
            MaxPossibleHeight += Layer.Amplitude;
        }
        if (MaxPossibleHeight == 0.0f) MaxPossibleHeight = 1000.0f; // Seguridad


        // Loop de vértices
        for (int32 i = 0; i < BaseVertices.Num(); i++)
        {
            FVector WorldPos = ComponentTransform.TransformPosition(BaseVertices[i]);
            FVector NoiseDir = (WorldPos - PlanetCenter).GetSafeNormal();

            float X = NoiseDir.X;
            float Y = NoiseDir.Y;
            float Z = NoiseDir.Z;

            // Aplicar Domain Warp
            if (bUseDomainWarp)
            {
                float WarpX = WarpNoise.GetNoise(X, Y, Z) * DomainWarpStrength;
                float WarpY = WarpNoise.GetNoise(X + 31.7f, Y + 17.3f, Z + 47.1f) * DomainWarpStrength;
                float WarpZ = WarpNoise.GetNoise(X + 59.2f, Y + 11.8f, Z + 23.4f) * DomainWarpStrength;

                X += WarpX;
                Y += WarpY;
                Z += WarpZ;
            }

            float TotalNoise = 0.0f;

            // Sumar capas 
            for (int32 LayerIndex = 0; LayerIndex < Layers.Num(); LayerIndex++)
            {
                const FCosmicNoiseTypes& Layer = Layers[LayerIndex];
                FastNoiseLite& Noise = ConfiguredNoises[LayerIndex];

                float LayerNoise = Noise.GetNoise(X, Y, Z);
                TotalNoise += LayerNoise * Layer.Amplitude;
            }

            // Fallback si no hay capas
            if (Layers.Num() == 0)
            {
                FastNoiseLite DefaultNoise;
                DefaultNoise.SetSeed(Seed);
                DefaultNoise.SetFrequency(0.001f);
                TotalNoise = DefaultNoise.GetNoise(X, Y, Z) * 1000.0f;
            }

            CalculatedVertices[i] = BaseVertices[i] + (BaseNormals[i] * TotalNoise);

            // 1. Latitud (0.0 en el ecuador, 1.0 en los polos Z)
            float Latitude = FMath::Abs(NoiseDir.Z);
            float BaseTemp = 1.0f - Latitude; // Calor base

            // 2. Altitud (Penalización por montaña)
            // Clamp para asegurar que no nos salimos de 0 a 1
            float AltitudePenalty = FMath::Clamp(TotalNoise / MaxPossibleHeight, 0.0f, 1.0f);
            BaseTemp -= (AltitudePenalty * 0.6f); // Las montañas restan hasta un 60% de temperatura

            // 3. Ruido térmico (para que no sean franjas perfectas)
            // Multiplicamos el NoiseDir por una frecuencia para el muestreo
            float TempVariance = TempNoise.GetNoise(NoiseDir.X * 100.0f, NoiseDir.Y * 100.0f, NoiseDir.Z * 100.0f) * 0.2f;
            float FinalTemp = FMath::Clamp(BaseTemp + TempVariance, 0.0f, 1.0f);

            // 4. Humedad (Fractal de nubes)
            float RawHum = HumidityNoise.GetNoise(NoiseDir.X * 100.0f, NoiseDir.Y * 100.0f, NoiseDir.Z * 100.0f);
            float FinalHum = (RawHum + 1.0f) * 0.5f; // Convertir de [-1, 1] a [0, 1]
            FinalHum = FMath::Clamp((FinalHum - 0.5f) * 1.5f + 0.5f, 0.0f, 1.0f);//Aumento de contraste

            // 5. Guardar en el array de Vertex Colors
            // R = Temperatura, G = Humedad, B = Altitud (Útil para mezclar nieve en el material)
            CalculatedColors[i] = FLinearColor(FinalTemp, FinalHum, AltitudePenalty, 1.0f);
        }
    }
};