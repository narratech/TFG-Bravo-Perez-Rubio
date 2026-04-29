// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "CosmicNoiseTypes.generated.h"

UENUM(BlueprintType)
enum class ECosmicNoiseType : uint8
{
    Perlin,
    Simplex,
    Cellular,
    Value,
    Ridged
};

UENUM(BlueprintType)
enum class ECosmicFractalType : uint8
{
    None,
    FBM,
    Ridged,
    PingPong
};

/**
 * 
 */
USTRUCT(Blueprintable, BlueprintType)
struct COSMICARCHITECTNOISE_API FCosmicNoiseLayer 
{
	GENERATED_BODY()
	
public:
    UPROPERTY(EditAnywhere, Category = "NoiseLayer", BlueprintReadWrite)
    ECosmicNoiseType NoiseType = ECosmicNoiseType::Simplex;

    UPROPERTY(EditAnywhere, Category = "NoiseLayer", BlueprintReadWrite)
    ECosmicFractalType FractalType = ECosmicFractalType::FBM;

    UPROPERTY(EditAnywhere, Category = "NoiseLayer", BlueprintReadWrite, meta = (ClampMin = "0"))
    float Frequency = 0.001f;

    UPROPERTY(EditAnywhere, Category = "NoiseLayer", BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "12"))
    int32 Octaves = 5;

    UPROPERTY(EditAnywhere, Category = "NoiseLayer", BlueprintReadWrite, meta = (ClampMin = "0"))
    float Lacunarity = 2.0f;

    UPROPERTY(EditAnywhere, Category = "NoiseLayer", BlueprintReadWrite, meta = (ClampMin = "0"))
    float Persistence = 0.5f;

    UPROPERTY(EditAnywhere, Category = "NoiseLayer", BlueprintReadWrite, meta = (ClampMin = "0"))
    float Amplitude = 1.0f;

};

USTRUCT(Blueprintable, BlueprintType)
struct COSMICARCHITECTNOISE_API FCosmicNoiseDataLayer
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "NoiseLayer", BlueprintReadWrite, meta = (ClampMin = "0"))
    float Frequency = 0.001f;

    UPROPERTY(EditAnywhere, Category = "NoiseLayer", BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "12"))
    int32 Octaves = 5;

    UPROPERTY(EditAnywhere, Category = "NoiseLayer", BlueprintReadWrite, meta = (ClampMin = "0"))
    float Lacunarity = 2.0f;

    UPROPERTY(EditAnywhere, Category = "NoiseLayer", BlueprintReadWrite, meta = (ClampMin = "0"))
    float Persistence = 0.5f;

    UPROPERTY(EditAnywhere, Category = "NoiseLayer", BlueprintReadWrite, meta = (ClampMin = "0"))
    float Amplitude = 1.0f;
};

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

USTRUCT(BlueprintType)
struct COSMICARCHITECTNOISE_API FCosmicNoiseSimpleParameters
{
    GENERATED_BODY()

    /* MODE SWITCH */
    UPROPERTY(EditAnywhere, Category = "Mode")
    bool bUseAdvancedSettings = false;

    /* SIMPLE MODE */
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
};

USTRUCT(BlueprintType)
struct COSMICARCHITECTNOISE_API FCosmicNoiseCraterParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Craters", meta = (ClampMin = "0.0001", ClampMax = "100"))
    float CraterFrequency = 2.0f;

    UPROPERTY(EditAnywhere, Category = "Craters")
    float CraterDepth = 300.0f;

    UPROPERTY(EditAnywhere, Category = "Craters", meta = (ClampMin = "1", ClampMax = "8"))
    int32 CraterOctaves = 3;

    UPROPERTY(EditAnywhere, Category = "Craters", meta = (ClampMin = "0", ClampMax = "2"))
    float CraterRadiusMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Craters", meta = (ClampMin = "0", ClampMax = "2"))
    float CraterRimHeight = 0.4f;

    UPROPERTY(EditAnywhere, Category = "Craters", meta = (ClampMin = "0.1", ClampMax = "20"))
    float CraterRimSharpness = 2.5f;

    UPROPERTY(EditAnywhere, Category = "Craters", meta = (ClampMin = "0", ClampMax = "1"))
    float CraterFloorHeight = 0.f;

    UPROPERTY(EditAnywhere, Category = "Craters", meta = (ClampMin = "0", ClampMax = "1"))
    float CraterDistortion = 0.15f;

    UPROPERTY(EditAnywhere, Category = "Craters", meta = (ClampMin = "0", ClampMax = "10"))
    float CraterLacunarity = 2.5f;

    UPROPERTY(EditAnywhere, Category = "Craters", meta = (ClampMin = "0", ClampMax = "1"))
    float CraterPersistence = 0.5f;

    UPROPERTY(EditAnywhere, Category = "Craters", meta = (ClampMin = "0", ClampMax = "1"))
    float CraterNoiseBreakup = 0.2f;
};

USTRUCT(BlueprintType)
struct COSMICARCHITECTNOISE_API FCosmicNoiseBiomeParameters
{
    GENERATED_BODY()

    /** Frecuencia para el ruido de temperatura (0.001-0.02) */
    UPROPERTY(EditAnywhere, Category = "Biome", meta = (ClampMin = "0.0001"))
    float TemperatureFrequency = 0.005f;

    /** Intensidad del efecto de latitud (0 = solo ruido, 1 = fuerte gradiente polar) */
    UPROPERTY(EditAnywhere, Category = "Biome", meta = (ClampMin = "0", ClampMax = "2"))
    float LatitudeEffect = 1.0f;

    /** Penalización de temperatura por altitud (0-1) */
    UPROPERTY(EditAnywhere, Category = "Biome", meta = (ClampMin = "0", ClampMax = "1"))
    float AltitudeTemperaturePenalty = 0.6f;

    /** Frecuencia para el ruido de humedad (0.001-0.1) */
    UPROPERTY(EditAnywhere, Category = "Biome", meta = (ClampMin = "0.0001"))
    float HumidityFrequency = 0.015f;

    /** Octavas para humedad (más = nubes más detalladas) */
    UPROPERTY(EditAnywhere, Category = "Biome", meta = (ClampMin = "1", ClampMax = "8"))
    int32 HumidityOctaves = 5;

    /** Contraste de humedad (1 = normal, >1 = más extremo) */
    UPROPERTY(EditAnywhere, Category = "Biome", meta = (ClampMin = "0.5", ClampMax = "3"))
    float HumidityContrast = 1.5f;

    /** Offset de humedad (para desplazar el rango) */
    UPROPERTY(EditAnywhere, Category = "Biome", meta = (ClampMin = "-1", ClampMax = "1"))
    float HumidityOffset = -0.5f;
};

USTRUCT(BlueprintType)
struct COSMICARCHITECTNOISE_API FCosmicNoiseDomainWarpParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "DomainWarp")
    bool bUseDomainWarp = false;

    UPROPERTY(EditAnywhere, Category = "DomainWarp")
    float DomainWarpStrength = 1000.0f;

    UPROPERTY(EditAnywhere, Category = "DomainWarp")
    float DomainWarpFrequency = 0.001f;
};
