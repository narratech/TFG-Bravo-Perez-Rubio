// Copyright 1998 - 2026 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CosmicNoiseClass.h"
#include "CosmicDefaultNoiseSettings.generated.h"

/**
 * Configuración utilizada para crear una estrategia de ruido procedural estándar.
 *
 * Esta clase encapsula los parámetros necesarios para inicializar
 * una instancia de FCosmicDefaultNoiseStrategy.
 */
UCLASS()
class COSMICARCHITECTNOISE_API UCosmicDefaultNoiseSettings : public UCosmicNoiseClass
{
    GENERATED_BODY()

public:

    /**
     * Semilla utilizada para la generación procedural del ruido.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    int32 Seed;

    /**
     * Parámetros generales de la capa de ruido principal.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseLayer LayerParameters;

    /**
     * Parámetros utilizados para la generación y mezcla de biomas.
     */
    UPROPERTY(EditAnywhere, Category = "Noise Settings")
    FCosmicNoiseBiomeParameters BiomeParameters;

    /**
     * Crea e inicializa la estrategia de ruido por defecto
     * utilizando la configuración actual.
     *
     * @return Estrategia de ruido completamente inicializada.
     */
    virtual TSharedPtr<ICosmicNoiseStrategy> CreateStrategy() const override;
};