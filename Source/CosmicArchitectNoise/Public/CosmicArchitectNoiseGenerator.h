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

    bool bUseCraters = false;
    float CraterFrequency;
    float CraterDepth;
    float CraterRadiusMultiplier;
    float CraterRimHeight;
    float CraterRimSharpness;
    float CraterFloorHeight;
    float CraterDistortion;
    int32 CraterOctaves;
    float CraterLacunarity;
    float CraterPersistence;
    float CraterNoiseBreakup;

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
        float InHumidityOffset,

        /* CRATERS */
        bool InUseCraters,
        float InCraterFrequency,
        float InCraterDepth,
        float InCraterRadiusMultiplier,
        float InCraterRimHeight,
        float InCraterRimSharpness,
        float InCraterFloorHeight,
        float InCraterDistortion,
        int32 InCraterOctaves,
        float InCraterLacunarity,
        float InCraterPersistence,
        float InCraterNoiseBreakup
    )
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

        /* CRATER INIT */
        , bUseCraters(InUseCraters)
        , CraterFrequency(InCraterFrequency)
        , CraterDepth(InCraterDepth)
        , CraterRadiusMultiplier(InCraterRadiusMultiplier)
        , CraterRimHeight(InCraterRimHeight)
        , CraterRimSharpness(InCraterRimSharpness)
        , CraterFloorHeight(InCraterFloorHeight)
        , CraterDistortion(InCraterDistortion)
        , CraterOctaves(InCraterOctaves)
        , CraterLacunarity(InCraterLacunarity)
        , CraterPersistence(InCraterPersistence)
        , CraterNoiseBreakup(InCraterNoiseBreakup)
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
        FastNoiseLite CraterNoise;

        HumidityNoise.SetSeed(Seed);
        HumidityNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        HumidityNoise.SetFractalType(FastNoiseLite::FractalType_FBm);

        // Usar los valores de los settings
        HumidityNoise.SetFrequency(HumidityFrequency * 100.0f);
        HumidityNoise.SetFractalOctaves(HumidityOctaves); 

        TempNoise.SetSeed(Seed);
        TempNoise.SetFrequency(TemperatureFrequency * 100.0f);

        if (bUseCraters)
        {
            CraterNoise.SetSeed(Seed + 4242);
            CraterNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
            CraterNoise.SetFrequency(CraterFrequency);
            CraterNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
            CraterNoise.SetFractalOctaves(CraterOctaves);
            CraterNoise.SetFractalLacunarity(CraterLacunarity);
            CraterNoise.SetFractalGain(CraterPersistence);
            CraterNoise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean);
            CraterNoise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance);
        }

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

        //Funcion de perfil de crateres
        auto CraterProfile = [&](float d)
            {
                if (d >= 1.5f)
                    return 0.0f;

                // cavidad
                float cavity = -(1.0f - d * d);

                // rim
                float rim = FMath::Exp(
                    -FMath::Pow((d - 1.0f) * CraterRimSharpness, 2.0f)
                ) * CraterRimHeight;

                float crater = cavity + rim;

                crater = FMath::Max(crater, CraterFloorHeight);

                return crater;
            };

        const int32 VertexCount = BaseVertices.Num();
        const bool bHasLayers = Layers.Num() > 0;

        // Loop de vértices
        for (int32 i = 0; i < VertexCount; i++)
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
            if (!bHasLayers)
            {
                FastNoiseLite DefaultNoise;
                DefaultNoise.SetSeed(Seed);
                DefaultNoise.SetFrequency(0.001f);
                TotalNoise = DefaultNoise.GetNoise(X, Y, Z) * 1000.0f;
            }

            if (bUseCraters)
            {
                float craterHeight = 0.0f;

                // distancia al centro de celda
                float d = FMath::Abs(CraterNoise.GetNoise(X, Y, Z)) * CraterRadiusMultiplier;

                // distorsión controlada
                if (CraterDistortion > 0.0f)
                {
                    float distort = ConfiguredNoises.Num() > 0
                        ? ConfiguredNoises[0].GetNoise(X, Y, Z)
                        : TempNoise.GetNoise(X, Y, Z);

                    d += distort * CraterDistortion;
                }

                float crater = CraterProfile(d);

                // floorHeight solo dentro del cráter
                if (d < 1.0f)
                {
                    crater = FMath::Max(crater, CraterFloorHeight);
                }

                if (CraterNoiseBreakup > 0.0f)
                {
                    float breakup = TempNoise.GetNoise(X * 4, Y * 4, Z * 4);
                    crater *= 1.0f + breakup * CraterNoiseBreakup;
                }

                // aplicar profundidad y hacia adentro
                craterHeight = -crater * CraterDepth;

                // superposición realista: los impactos nuevos cortan los antiguos
                TotalNoise += craterHeight;
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
                NoiseDir.X,
                NoiseDir.Y,
                NoiseDir.Z
            ) * 0.2f;

            float FinalTemp = FMath::Clamp(BaseTemp + TempVariance, 0.0f, 1.0f);

            // Humedad 
            float RawHum = HumidityNoise.GetNoise(
                NoiseDir.X,
                NoiseDir.Y,
                NoiseDir.Z
            );

            // Convertir de [-1, 1] a [0, 1] y aplicar offset
            float FinalHum = (RawHum + 1.0f) * 0.5f + HumidityOffset;  
            FinalHum = FMath::Clamp((FinalHum - 0.5f) * HumidityContrast + 0.5f, 0.0f, 1.0f); 

            // Guardar colores
            CalculatedColors[i] = FLinearColor(FinalTemp, FinalHum, AltitudePenalty, 1.0f);
        }
    }
};