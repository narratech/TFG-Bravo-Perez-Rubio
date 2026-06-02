// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CosmicNoiseClass.h"
#include "CosmicRealisticNoiseSettings.generated.h"

/**
 * 
 */
UCLASS()
class COSMICARCHITECTNOISE_API UCosmicRealisticNoiseSettings : public UCosmicNoiseClass
{
	GENERATED_BODY()
	
    /**
     * Semilla utilizada para la generaci�n procedural del terreno.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    int32 Seed;

    /**
     * Escala utilizada para normalizar la altura final del terreno.
     *
     * Valores m�s bajos comprimen las alturas.
     * Valores m�s altos preservan mayor variaci�n vertical.
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
     * Capa utilizada para generar cadenas monta�osas.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseDataLayer MountainLayer;

    /**
     * Capa utilizada para generar colinas y variaciones suaves.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseDataLayer HillLayer;

    /**
     * Capa utilizada para a�adir detalle fino al terreno.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseDataLayer DetailLayer;

    /**
     * Capa utilizada para la generaci�n procedural de r�os.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseDataLayer RiverLayer;

    /**
     * Par�metros relacionados con la generaci�n de biomas.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseBiomeParameters BiomeParameters;

    /**
     * Crea e inicializa la estrategia de ruido tipo Tierra
     * utilizando la configuraci�n actual.
     *
     * @return Estrategia de ruido completamente inicializada.
     */
    virtual TSharedPtr<ICosmicNoiseStrategy> CreateStrategy() const override;
};
