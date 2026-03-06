// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CosmicNoiseTypes.h"
#include "CosmicNoiseSettings.generated.h"

UENUM()
enum class ECosmicBiomeType : uint8
{
    TemperateForest    UMETA(DisplayName = "Temperate Forest"),
    Rainforest         UMETA(DisplayName = "Rainforest"),
    Desert             UMETA(DisplayName = "Desert"),
    Tundra             UMETA(DisplayName = "Tundra"),
    Taiga              UMETA(DisplayName = "Taiga"),
    Savannah           UMETA(DisplayName = "Savannah"),
    Grassland          UMETA(DisplayName = "Grassland"),
    Swamp              UMETA(DisplayName = "Swamp"),
    Volcanic           UMETA(DisplayName = "Volcanic"),
    Alien              UMETA(DisplayName = "Alien"),
    Ocean              UMETA(DisplayName = "Ocean"),
    Ice                UMETA(DisplayName = "Ice"),
    Cratered           UMETA(DisplayName = "Cratered Moon")
};

/**
 * 
 */
UCLASS(BlueprintType)
class COSMICARCHITECTNOISE_API UCosmicNoiseSettings : public UDataAsset
{
	GENERATED_BODY()
	
public:

    UCosmicNoiseSettings();

    /* PUBLIC METHODS */
    /** Convierte los parámetros simples a capas avanzadas */
    UFUNCTION(BlueprintCallable, Category = "Noise")
    void UpdateAdvancedFromSimple();

    UFUNCTION(BlueprintCallable, Category = "Noise")
    void UpdateBiomesFromSimple();

    UFUNCTION(BlueprintCallable, Category = "Noise")
    void UpdateNoiseFromSimple();

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    /* MODE SWITCH */

    UPROPERTY(EditAnywhere, Category = "Mode")
    bool bUseAdvancedSettings = false;

    UPROPERTY(EditAnywhere, Category = "Simple")
    bool bIsCraterPlanet = false;

    /* SIMPLE MODE */

    UPROPERTY(EditAnywhere, Category = "Simple")
    int32 Seed = 1337;

    UPROPERTY(EditAnywhere, Category = "Simple", meta = (EditCondition = "!bUseAdvancedSettings", ClampMin = "0"))
    float MaxMountainHeight = 3000.0f;

    UPROPERTY(EditAnywhere, Category = "Simple", meta = (EditCondition = "!bUseAdvancedSettings", ClampMin = "0", ClampMax = "1"))
    float Mountainous = 0.6f;

    UPROPERTY(EditAnywhere, Category = "Simple", meta = (EditCondition = "!bUseAdvancedSettings", ClampMin = "0", ClampMax = "1"))
    float Roughness = 0.4f;

    UPROPERTY(EditAnywhere, Category = "Simple", meta = (EditCondition = "!bUseAdvancedSettings", ClampMin = "0", ClampMax = "1"))
    float Detail = 0.7f;

    UPROPERTY(EditAnywhere, Category = "Simple", meta = (EditCondition = "!bUseAdvancedSettings", ClampMin = "0", ClampMax = "1"))
    float Smoothness = 0.5f;

    UPROPERTY(EditAnywhere, Category = "Simple", meta = (EditCondition = "!bUseAdvancedSettings"))
    ECosmicBiomeType BiomeType = ECosmicBiomeType::Desert;

        /* ADVANCED MODE */

    UPROPERTY(EditAnywhere, Category = "Advanced", meta = (EditCondition = "bUseAdvancedSettings"))
    TArray<FCosmicNoiseTypes> NoiseLayers;

    UPROPERTY(EditAnywhere, Category = "Advanced", meta = (EditCondition = "bUseAdvancedSettings"))
    bool bUseDomainWarp = false;

    UPROPERTY(EditAnywhere, Category = "Advanced", meta = (EditCondition = "bUseAdvancedSettings"))
    float DomainWarpStrength = 1000.0f;

    UPROPERTY(EditAnywhere, Category = "Advanced", meta = (EditCondition = "bUseAdvancedSettings"))
    float DomainWarpFrequency = 0.001f;

    /** Frecuencia para el ruido de temperatura (0.001-0.02) */
    UPROPERTY(EditAnywhere, Category = "Biome", meta = (ClampMin = "0.0001", ClampMax = "0.1", EditCondition = "bUseAdvancedSettings"))
    float TemperatureFrequency = 0.005f;

    /** Frecuencia para el ruido de humedad (0.001-0.1) */
    UPROPERTY(EditAnywhere, Category = "Biome", meta = (ClampMin = "0.0001", ClampMax = "0.1", EditCondition = "bUseAdvancedSettings"))
    float HumidityFrequency = 0.015f;

    /** Octavas para humedad (más = nubes más detalladas) */
    UPROPERTY(EditAnywhere, Category = "Biome", meta = (ClampMin = "1", ClampMax = "8", EditCondition = "bUseAdvancedSettings"))
    int32 HumidityOctaves = 5;

    /** Intensidad del efecto de latitud (0 = solo ruido, 1 = fuerte gradiente polar) */
    UPROPERTY(EditAnywhere, Category = "Biome", meta = (ClampMin = "0", ClampMax = "2", EditCondition = "bUseAdvancedSettings"))
    float LatitudeEffect = 1.0f;

    /** Penalización de temperatura por altitud (0-1) */
    UPROPERTY(EditAnywhere, Category = "Biome", meta = (ClampMin = "0", ClampMax = "1", EditCondition = "bUseAdvancedSettings"))
    float AltitudeTemperaturePenalty = 0.6f;

    /** Contraste de humedad (1 = normal, >1 = más extremo) */
    UPROPERTY(EditAnywhere, Category = "Biome", meta = (ClampMin = "0.5", ClampMax = "3", EditCondition = "bUseAdvancedSettings"))
    float HumidityContrast = 1.5f;

    /** Offset de humedad (para desplazar el rango) */
    UPROPERTY(EditAnywhere, Category = "Biome", meta = (ClampMin = "-1", ClampMax = "1", EditCondition = "bUseAdvancedSettings"))
    float HumidityOffset = -0.5f;
};
