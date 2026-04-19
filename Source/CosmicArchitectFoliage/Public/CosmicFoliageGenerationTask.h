// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CosmicFoliageCollection.h"
#include "CosmicArchitectNoise/Public/CosmicNoiseSettings.h"
#include "CosmicArchitectNoise/Public/CosmicNoiseEvaluator.h"
#include "CosmicArchitectCommon/Public/CosmicCubeMapCell.h"
#include "CosmicFoliageGenerationTask.generated.h"

class ICosmicNoiseStrategy;

/**
 * 
 */
USTRUCT()
struct FCosmicFoliageInstance
{
    GENERATED_BODY()

    UPROPERTY()
    UStaticMesh* Mesh = nullptr;

    UPROPERTY()
    FTransform Transform;
};

/**
 * Tarea asincrona para calcular posiciones de foliage
 */
class FFoliageGenerationTask : public FNonAbandonableTask
{
public:
    TArray<FCosmicFoliageInstance> ResultInstances;
    FCubeMapCell Cell;
    ECosmicFoliageLayer Layer;

    // Almacenamos los puntos de generación con su información ambiental
    struct FSeedPoint
    {
        FVector Direction;      // Dirección desde el centro del planeta
        FVector WorldPosition;  // Posición en el mundo
        float Temperature;
        float Humidity;
        float Height;
        float Slope;
    };

    FFoliageGenerationTask(
        const FCubeMapCell& InCell,
        ECosmicFoliageLayer InLayer,
        UCosmicFoliageCollection* InCollection,
        const FVector& InPlanetCenter,
        float InPlanetRadius,
        TSharedPtr<ICosmicNoiseStrategy> InNoiseGenerationStrategy,
        float InCellAreaKm2)
        : Cell(InCell)
        , Layer(InLayer)
        , Collection(InCollection)
        , PlanetCenter(InPlanetCenter)
        , PlanetRadius(InPlanetRadius)
        , NoiseGenerationStrategy(InNoiseGenerationStrategy)
        , CellAreaKm2(InCellAreaKm2)
    {
    }


    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(FFoliageGenerationTask, STATGROUP_ThreadPoolAsyncTasks);
    }

    void DoWork();

private:

    UCosmicFoliageCollection* Collection;
    FVector PlanetCenter;
    float PlanetRadius;
    TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy;
    float CellAreaKm2;
    TArray<FSeedPoint> SeedPoints;

    void GenerateSeedPoints(FRandomStream& Random);
    void EvaluateEnvironmentalConditions(FRandomStream& Random);
    float CalculateSlope(const FVector& Direction, int32 PointIndex);
    void CreateFoliageInstances(FRandomStream& Random);
    const FCosmicFoliageCollectionEntry* FindBestMatchingEntry(float Temperature, float Humidity, float Slope, float Height);
    const FCosmicFoliageCollectionEntry* FindClosestMatchingEntry( float Temperature, float Humidity, float Slope, float Height);
    FVector GetTerrainNormal(const FVector& Direction);
};
