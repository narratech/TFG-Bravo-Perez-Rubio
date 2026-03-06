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
struct COSMICARCHITECTNOISE_API FCosmicNoiseTypes 
{
	GENERATED_BODY()
	
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECosmicNoiseType NoiseType = ECosmicNoiseType::Simplex;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECosmicFractalType FractalType = ECosmicFractalType::FBM;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
    float Frequency = 0.001f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "12"))
    int32 Octaves = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
    float Lacunarity = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
    float Persistence = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
    float Amplitude = 1.0f;

};
