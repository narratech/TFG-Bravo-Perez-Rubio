// Copyright 1998 - 2026 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ICosmicNoiseStrategy.h"
#include "ThirdParty/FastNoiseLite.h"

/**
 * Estrategia de ruido procedural estándar utilizada para
 * generar superficies planetarias con biomas dinámicos.
 *
 * Esta implementación combina:
 * - Ruido base procedural.
 * - Variaciones de humedad.
 * - Variaciones de temperatura.
 * - Influencia de biomas sobre la altura final.
 */
class COSMICARCHITECTNOISE_API FCosmicDefaultNoiseStrategy : public ICosmicNoiseStrategy
{
public:

    /**
     * Semilla utilizada para inicializar todos los generadores de ruido.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    int32 Seed;

    /**
     * Parámetros generales de la capa principal de ruido.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseLayer LayerParameters;

    /**
     * Parámetros utilizados para la generación de biomas.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseBiomeParameters BiomeParameters;

    /**
     * Inicializa la estrategia de ruido y configura todos
     * los generadores internos necesarios.
     *
     * @param Seed Semilla procedural utilizada para los ruidos.
     * @param LayerParameters Configuración del ruido base.
     * @param BiomeParameters Configuración de biomas.
     */
    void Initialize(
        int32 Seed,
        FCosmicNoiseLayer LayerParameters,
        FCosmicNoiseBiomeParameters BiomeParameters
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
};