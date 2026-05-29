// Copyright 1998 - 2026 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ICosmicNoiseStrategy.h"
#include "ThirdParty/FastNoiseLite.h"

/**
 * Estrategia de generación de ruido procedural tipo “Earth-like”.
 *
 * Combina múltiples capas de ruido (continentes, montañas, colinas, ríos y detalle)
 * junto con parámetros de bioma (humedad y temperatura) para generar terreno planetario.
 */
class COSMICARCHITECTNOISE_API FCosmicEarthLikeNoiseStrategy : public ICosmicNoiseStrategy
{
public:

    /** Semilla base utilizada para inicializar todos los sistemas de ruido */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    int32 Seed;

    /** Factor global de normalización de altura del terreno */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    float HeightNormalizationScale = 1.0f;

    /** Capa de ruido principal que define continentes y masas terrestres */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseDataLayer ContinentalLayer;

    /** Capa de ruido utilizada para la generación de montañas */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseDataLayer MountainLayer;

    /** Capa de ruido utilizada para la generación de colinas */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseDataLayer HillLayer;

    /** Capa de ruido de detalle fino del terreno */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseDataLayer DetailLayer;

    /** Capa de ruido utilizada para la generación de ríos */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseDataLayer RiverLayer;

    /** Parámetros globales de biomas (humedad, temperatura, etc.) */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseBiomeParameters BiomeParameters;

    /**
     * Inicializa la estrategia de ruido con todos los parámetros necesarios.
     */
    void Initialize(
        int32 InSeed,
        float InHeightNormalizationScale,
        FCosmicNoiseBiomeParameters InBiomeParameters,
        FCosmicNoiseDataLayer InContinental,
        FCosmicNoiseDataLayer InMountain,
        FCosmicNoiseDataLayer InHill,
        FCosmicNoiseDataLayer InDetail,
        FCosmicNoiseDataLayer InRiver);

    /**
     * Evalúa un punto en el espacio de ruido y genera altura y color de bioma.
     */
    void EvaluatePoint(const FVector& NoiseDir, float& OutHeight, FLinearColor& OutColor) const override;

protected:

    /** Ruido base de continentes */
    FastNoiseLite ContinentalNoise;

    /** Ruido de montañas (ridged noise) */
    FastNoiseLite MountainNoise;

    /** Ruido de colinas */
    FastNoiseLite HillNoise;

    /** Ruido de detalle fino */
    FastNoiseLite DetailNoise;

    /** Ruido de ríos basado en cellular noise */
    FastNoiseLite RiverNoise;

    /** Ruido de humedad para biomas */
    FastNoiseLite HumidityNoise;

    /** Ruido de temperatura para biomas */
    FastNoiseLite TempNoise;
};