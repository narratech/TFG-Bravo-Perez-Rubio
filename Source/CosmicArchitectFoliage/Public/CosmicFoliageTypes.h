// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "CosmicCubeMapCell.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "CosmicFoliageTypes.generated.h"

UENUM(BlueprintType)
enum class ECosmicFoliagePlacement : uint8
{
    Ground      UMETA(DisplayName = "Ground"),
    Cliff       UMETA(DisplayName = "Cliff"),
    Water       UMETA(DisplayName = "Water"),
    Any         UMETA(DisplayName = "Any")
};

UENUM(BlueprintType)
enum class ECosmicFoliageLayer : uint8 
{
    Near      UMETA(DisplayName = "Near"),
    Medium    UMETA(DisplayName = "Medium"),
    Far       UMETA(DisplayName = "Far")
};

USTRUCT(BlueprintType)
struct COSMICARCHITECTFOLIAGE_API FCosmicFoliageMesh
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage Settings")
    UStaticMesh* Mesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage Settings")
    ECosmicFoliageLayer FoliageLayer = ECosmicFoliageLayer::Medium;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage Settings", meta = (ClampMin = "1", ClampMax = "200000"))
    int32 InstancesPerKm2 = 1000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage Settings", meta = (ClampMin = "0.1", ClampMax = "150"))
    float ScaleMin = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage Settings", meta = (ClampMin = "0.1", ClampMax = "150"))
    float ScaleMax = 1.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage Settings")
    FVector HeightOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage Settings")
    bool bAlignToGround = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage Settings")
    bool bAlignToPlanetNormal = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage Settings", meta = (ClampMin = "-180", ClampMax = "180"))
    float RandomRotationMin = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage Settings", meta = (ClampMin = "-180", ClampMax = "180"))
    float RandomRotationMax = 360.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage Settings")
    bool bHasCollision = true;
};

USTRUCT(BlueprintType)
struct COSMICARCHITECTFOLIAGE_API FCosmicFoliageCollectionEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlacementRules")
    TArray<FCosmicFoliageMesh> Foliage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlacementRules", meta = (ClampMin = "-90", ClampMax = "90"))
    float SlopeMin = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlacementRules", meta = (ClampMin = "-90", ClampMax = "90"))
    float SlopeMax = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlacementRules", meta = (ClampMin = "-100", ClampMax = "1000"))
    float ElevationMinKm = -0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlacementRules", meta = (ClampMin = "-100", ClampMax = "1000"))
    float ElevationMaxKm = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlacementRules", meta = (ClampMin = "0", ClampMax = "1"))
    float TemperatureMin = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlacementRules", meta = (ClampMin = "0", ClampMax = "1"))
    float TemperatureMax = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlacementRules", meta = (ClampMin = "0", ClampMax = "1"))
    float HumidityMin = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlacementRules", meta = (ClampMin = "0", ClampMax = "1"))
    float HumidityMax = 1.0f;
};

USTRUCT()
struct FCosmicHISMKey
{
    GENERATED_BODY()

    UStaticMesh* Mesh = nullptr;
    bool         bHasCollision = true;

    bool operator==(const FCosmicHISMKey& Other) const
    {
        return Mesh == Other.Mesh && bHasCollision == Other.bHasCollision;
    }
};

FORCEINLINE uint32 GetTypeHash(const FCosmicHISMKey& Key)
{
    return HashCombine(GetTypeHash(Key.Mesh), GetTypeHash(Key.bHasCollision));
}

USTRUCT()
struct FCosmicFoliageCellData
{
    GENERATED_BODY()

    TMap<FCosmicHISMKey, UHierarchicalInstancedStaticMeshComponent*> MeshComponents;
};

// Estructura para almacenar celdas por capa
USTRUCT()
struct FCosmicFoliageLayerCells
{
    GENERATED_BODY()

    // Mapa de celda: datos de componentes para esta capa específica
    TMap<FCubeMapCell, FCosmicFoliageCellData> ActiveCells;
};

USTRUCT()
struct FCosmicHISMPoolList
{
    GENERATED_BODY()

    TArray<UHierarchicalInstancedStaticMeshComponent*> Components;
};

struct FCosmicHISMPoolKey
{
    UStaticMesh* Mesh;
    ECosmicFoliageLayer Layer;

    bool operator==(const FCosmicHISMPoolKey& Other) const
    {
        return Mesh == Other.Mesh && Layer == Other.Layer;
    }
};

FORCEINLINE uint32 GetTypeHash(const FCosmicHISMPoolKey& Key)
{
    return HashCombine(GetTypeHash(Key.Mesh), (uint32)Key.Layer);
}


