#pragma once

#include "CoreMinimal.h"
#include "CosmicNoiseSettings.h"
#include "ThirdParty/FastNoiseLite.h"

struct COSMICARCHITECTNOISE_API FCosmicNoiseEvaluator
{
public:
    FCosmicNoiseEvaluator(FCosmicNoiseGenerationParameters InSettings);
    FCosmicNoiseEvaluator();
    // Copia de los ajustes para tenerlos a mano
    FCosmicNoiseGenerationParameters Settings;

    // Generadores de ruido globales pre-configurados
    FastNoiseLite HumidityNoise;
    FastNoiseLite TempNoise;
    FastNoiseLite CraterNoise;
    FastNoiseLite WarpNoise;

    // Matriz de ruidos para los biomas: [IndiceBioma][IndiceCapa]
    TArray<TArray<FastNoiseLite>> BiomeNoises;

    // Altura máxima pre-calculada para la penalización de temperatura
    float MaxPossibleHeight = 0.0f;

    void UpdateSettings(FCosmicNoiseGenerationParameters InSettings);
    // Devuelve la Altura y el Color exactos para una dirección dada
    void EvaluatePoint(const FVector& NoiseDir, float& OutHeight, FLinearColor& OutColor);
};