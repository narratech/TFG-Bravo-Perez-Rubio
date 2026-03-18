// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "CosmicFoliageTypes.generated.h"


UENUM(BlueprintType)
enum class ECosmicFoliagePlacement : uint8
{
    Ground      UMETA(DisplayName = "Ground"),
    Cliff       UMETA(DisplayName = "Cliff"),
    Water       UMETA(DisplayName = "Water"),
    Any         UMETA(DisplayName = "Any")
};

USTRUCT(BlueprintType)
struct COSMICARCHITECTFOLIAGE_API FCosmicFoliageMesh
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage Settings")
    UStaticMesh* Mesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage Settings", meta = (ClampMin = "0.1", ClampMax = "10"))
    float ScaleMin = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage Settings", meta = (ClampMin = "0.1", ClampMax = "10"))
    float ScaleMax = 1.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage Settings")
    bool bAlignToGround = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage Settings", meta = (ClampMin = "-180", ClampMax = "180"))
    float RandomRotationMin = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage Settings", meta = (ClampMin = "-180", ClampMax = "180"))
    float RandomRotationMax = 360.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage Settings")
    bool bCanSpawnInWater = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage Settings", meta = (ClampMin = "0", ClampMax = "1"))
    float DensityMultiplier = 1.0f;
};

USTRUCT(BlueprintType)
struct COSMICARCHITECTFOLIAGE_API FCosmicFoliageCollectionEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlacementRules")
    TArray<FCosmicFoliageMesh> Foliage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlacementRules", meta = (ClampMin = "0", ClampMax = "1"))
    float Weight = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlacementRules", meta = (ClampMin = "-90", ClampMax = "90"))
    float SlopeMin = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlacementRules", meta = (ClampMin = "-90", ClampMax = "90"))
    float SlopeMax = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlacementRules", meta = (ClampMin = "-1000", ClampMax = "10000"))
    float ElevationMin = -100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlacementRules", meta = (ClampMin = "-1000", ClampMax = "10000"))
    float ElevationMax = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlacementRules", meta = (ClampMin = "0", ClampMax = "1"))
    float TemperatureMin = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlacementRules", meta = (ClampMin = "0", ClampMax = "1"))
    float TemperatureMax = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlacementRules", meta = (ClampMin = "0", ClampMax = "1"))
    float HumidityMin = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlacementRules", meta = (ClampMin = "0", ClampMax = "1"))
    float HumidityMax = 1.0f;
};


