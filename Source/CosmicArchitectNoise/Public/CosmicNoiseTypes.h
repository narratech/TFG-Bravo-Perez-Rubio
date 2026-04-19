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
