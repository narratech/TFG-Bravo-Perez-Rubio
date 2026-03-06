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
    float TemperatureFrequency;
    float HumidityFrequency;
    int32 HumidityOctaves;
    float LatitudeEffect;
    float AltitudeTemperaturePenalty;
    float HumidityContrast;
    float HumidityOffset;

    FCosmicArchitectNoiseGenerator(
        const TArray<FVector>& InBaseVerts,
        const TArray<FVector>& InBaseNormals,
        FTransform InTransform,
        FVector InPlanetCenter,
        int32 InSeed,
        const TArray<FCosmicNoiseTypes>& InLayers,
        bool InUseWarp,
        float InWarpStrength,
        float InWarpFreq,
        float InTemperatureFrequency,
        float InHumidityFrequency,
        int32 InHumidityOctaves,
        float InLatitudeEffect,
        float InAltitudeTemperaturePenalty,
        float InHumidityContrast,
        float InHumidityOffset)
        : BaseVertices(InBaseVerts)
        , BaseNormals(InBaseNormals)
        , ComponentTransform(InTransform)
        , PlanetCenter(InPlanetCenter)
        , Seed(InSeed)
        , Layers(InLayers)
        , bUseDomainWarp(InUseWarp)
        , DomainWarpStrength(InWarpStrength)
        , DomainWarpFrequency(InWarpFreq)
        , TemperatureFrequency(InTemperatureFrequency)
        , HumidityFrequency(InHumidityFrequency)
        , HumidityOctaves(InHumidityOctaves)
        , LatitudeEffect(InLatitudeEffect)
        , AltitudeTemperaturePenalty(InAltitudeTemperaturePenalty)
        , HumidityContrast(InHumidityContrast)
        , HumidityOffset(InHumidityOffset)
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

        HumidityNoise.SetSeed(Seed);
        HumidityNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        HumidityNoise.SetFractalType(FastNoiseLite::FractalType_FBm);

        // Usar los valores de los settings
        HumidityNoise.SetFrequency(HumidityFrequency); 
        HumidityNoise.SetFractalOctaves(HumidityOctaves); 

        TempNoise.SetSeed(Seed);
        TempNoise.SetFrequency(TemperatureFrequency);  

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

            // Latitud con intensidad 
            float Latitude = FMath::Abs(NoiseDir.Z);
            float BaseTemp = 1.0f - (Latitude * LatitudeEffect); 

            // Penalización por altitud 
            float AltitudePenalty = FMath::Clamp(TotalNoise / MaxPossibleHeight, 0.0f, 1.0f);
            BaseTemp -= (AltitudePenalty * AltitudeTemperaturePenalty);  

            // Ruido térmico
            float TempVariance = TempNoise.GetNoise(
                NoiseDir.X * 100.0f,
                NoiseDir.Y * 100.0f,
                NoiseDir.Z * 100.0f
            ) * 0.2f;

            float FinalTemp = FMath::Clamp(BaseTemp + TempVariance, 0.0f, 1.0f);

            // Humedad 
            float RawHum = HumidityNoise.GetNoise(
                NoiseDir.X * 100.0f,
                NoiseDir.Y * 100.0f,
                NoiseDir.Z * 100.0f
            );

            // Convertir de [-1, 1] a [0, 1] y aplicar offset
            float FinalHum = (RawHum + 1.0f) * 0.5f + HumidityOffset;  
            FinalHum = FMath::Clamp((FinalHum - 0.5f) * HumidityContrast + 0.5f, 0.0f, 1.0f); 

            // Guardar colores
            CalculatedColors[i] = FLinearColor(FinalTemp, FinalHum, AltitudePenalty, 1.0f);
        }
    }
};