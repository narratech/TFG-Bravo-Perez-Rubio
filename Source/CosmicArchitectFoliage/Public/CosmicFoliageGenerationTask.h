// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CosmicFoliageCollection.h"
#include "CosmicArchitectNoise/Public/CosmicNoiseSettings.h"
#include "CosmicArchitectCommon/Public/CubeMapCell.h"
#include "CosmicFoliageGenerationTask.generated.h"

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
    UCosmicFoliageCollection* Collection;
    int32 Seed;
    FVector PlanetCenter;
    float PlanetRadius;
    FCosmicNoiseGenerationParameters NoiseSettings;
    float CellAreaKm2;

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

    TArray<FSeedPoint> SeedPoints;

    FFoliageGenerationTask(
        const FCubeMapCell& InCell,
        UCosmicFoliageCollection* InCollection,
        int32 InSeed,
        const FVector& InPlanetCenter,
        float InPlanetRadius,
        FCosmicNoiseGenerationParameters InNoiseSettings,
        float InCellAreaKm2)
        : Cell(InCell)
        , Collection(InCollection)
        , Seed(InSeed)
        , PlanetCenter(InPlanetCenter)
        , PlanetRadius(InPlanetRadius)
        , NoiseSettings(InNoiseSettings)
        , CellAreaKm2(InCellAreaKm2)
    {
    }


    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(FFoliageGenerationTask, STATGROUP_ThreadPoolAsyncTasks);
    }

    void DoWork();

private:
    void GenerateSeedPoints(FRandomStream& Random);
    void EvaluateEnvironmentalConditions(FRandomStream& Random);
    float CalculateSlope(const FVector& Direction, int32 PointIndex, const TArray<float>& AllHeights, FRandomStream& Random);
    void CreateFoliageInstances(FRandomStream& Random);
    const FCosmicFoliageCollectionEntry* FindBestMatchingEntry(float Temperature, float Humidity, float Slope, float Height);
    const FCosmicFoliageCollectionEntry* FindClosestMatchingEntry( float Temperature, float Humidity, float Slope, float Height);
    FVector GetTerrainNormal(const FVector& Direction, FRandomStream& Random);
};
