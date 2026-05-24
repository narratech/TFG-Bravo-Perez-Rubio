// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CosmicNoiseClass.h"
#include "CosmicCraterNoiseSettings.generated.h"

/**
 * Configuración utilizada para generar una estrategia de ruido basada en cráteres.
 *
 * Esta clase encapsula todos los parámetros necesarios para inicializar
 * una instancia de FCosmicCraterNoiseStrategy.
 */
UCLASS()
class COSMICARCHITECTNOISE_API UCosmicCraterNoiseSettings : public UCosmicNoiseClass
{
    GENERATED_BODY()

public:

    /**
     * Semilla utilizada para la generación procedural del ruido.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    int32 Seed;

    /**
     * Parámetros generales de las capas de ruido.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseLayer LayerParameters;

    /**
     * Parámetros relacionados con la generación de biomas.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseBiomeParameters BiomeParameters;

    /**
     * Parámetros específicos para la generación de cráteres.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseCraterParameters CraterParameters;

    /**
     * Crea e inicializa la estrategia de ruido de cráteres
     * utilizando los parámetros configurados en esta clase.
     *
     * @return Estrategia de ruido completamente inicializada.
     */
    virtual TSharedPtr<ICosmicNoiseStrategy> CreateStrategy() const override;
};