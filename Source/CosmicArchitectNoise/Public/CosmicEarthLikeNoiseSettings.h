// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CosmicNoiseClass.h"
#include "CosmicEarthLikeNoiseSettings.generated.h"

/**
 * Configuración utilizada para generar una estrategia de ruido
 * orientada a planetas tipo Tierra.
 *
 * Esta configuración combina múltiples capas de ruido especializadas:
 * - Continentes.
 * - Montañas.
 * - Colinas.
 * - Detalle fino.
 * - Ríos.
 *
 * Además incluye parámetros de biomas y normalización de altura.
 */
UCLASS()
class COSMICARCHITECTNOISE_API UCosmicEarthLikeNoiseSettings : public UCosmicNoiseClass
{
    GENERATED_BODY()

public:

    /**
     * Semilla utilizada para la generación procedural del terreno.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    int32 Seed;

    /**
     * Escala utilizada para normalizar la altura final del terreno.
     *
     * Valores más bajos comprimen las alturas.
     * Valores más altos preservan mayor variación vertical.
     */
    UPROPERTY(
        EditAnywhere,
        Category = "Noise Settings",
        meta = (ClampMin = "0.0", ClampMax = "1.0")
    )
    float HeightNormalizationScale = 1.0f;

    /**
     * Capa principal utilizada para generar continentes.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseDataLayer ContinentalLayer;

    /**
     * Capa utilizada para generar cadenas montañosas.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseDataLayer MountainLayer;

    /**
     * Capa utilizada para generar colinas y variaciones suaves.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseDataLayer HillLayer;

    /**
     * Capa utilizada para añadir detalle fino al terreno.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseDataLayer DetailLayer;

    /**
     * Capa utilizada para la generación procedural de ríos.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseDataLayer RiverLayer;

    /**
     * Parámetros relacionados con la generación de biomas.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseBiomeParameters BiomeParameters;

    /**
     * Crea e inicializa la estrategia de ruido tipo Tierra
     * utilizando la configuración actual.
     *
     * @return Estrategia de ruido completamente inicializada.
     */
    virtual TSharedPtr<ICosmicNoiseStrategy> CreateStrategy() const override;
};