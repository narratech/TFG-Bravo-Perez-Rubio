// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ICosmicNoiseStrategy.h"
#include "ThirdParty/FastNoiseLite.h"

/**
 * Estrategia de generación de ruido procedural orientada
 * a superficies planetarias con cráteres.
 *
 * Esta implementación combina:
 * - Ruido base procedural.
 * - Generación de biomas.
 * - Variaciones de humedad y temperatura.
 * - Generación de cráteres multicapa.
 */
class COSMICARCHITECTNOISE_API FCosmicCraterNoiseStrategy : public ICosmicNoiseStrategy
{
public:

    /**
     * Semilla utilizada para inicializar todos los generadores de ruido.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    int32 Seed;

    /**
     * Parámetros generales de las capas de ruido base.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseLayer LayerParameters;

    /**
     * Parámetros relacionados con la generación de biomas.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseBiomeParameters BiomeParameters;

    /**
     * Parámetros específicos utilizados para la generación de cráteres.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseCraterParameters CraterParameters;

    /**
     * Inicializa la estrategia de ruido y configura todos los
     * generadores internos necesarios.
     *
     * @param Seed Semilla procedural utilizada para los ruidos.
     * @param LayerParameters Configuración del ruido base.
     * @param BiomeParameters Configuración de biomas.
     * @param CraterParameters Configuración de generación de cráteres.
     */
    void Initialize(
        int32 Seed,
        FCosmicNoiseLayer LayerParameters,
        FCosmicNoiseBiomeParameters BiomeParameters,
        FCosmicNoiseCraterParameters CraterParameters
    );

    /**
     * Evalúa un punto sobre la superficie procedural y calcula:
     * - Altura final.
     * - Color representativo del bioma.
     *
     * @param NoiseDir Dirección normalizada utilizada como coordenada de ruido.
     * @param OutHeight Altura resultante calculada.
     * @param OutColor Color asociado al bioma generado.
     */
    void EvaluatePoint(
        const FVector& NoiseDir,
        float& OutHeight,
        FLinearColor& OutColor
    ) const override;

protected:

    /**
     * Generador de ruido utilizado para calcular humedad.
     */
    FastNoiseLite HumidityNoise;

    /**
     * Generador de ruido utilizado para calcular temperatura.
     */
    FastNoiseLite TempNoise;

    /**
     * Generador principal de ruido base del terreno.
     */
    FastNoiseLite Noise;

    /**
     * Generador de ruido utilizado para la formación de cráteres.
     */
    FastNoiseLite CraterNoise;
};