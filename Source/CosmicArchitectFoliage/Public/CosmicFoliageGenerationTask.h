// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CosmicFoliageCollection.h"
#include "CosmicArchitectCommon/Public/CosmicCubeMapCell.h"
#include "CosmicFoliageGenerationTask.generated.h"

class ICosmicNoiseStrategy;

/**
 * Generated foliage instance resulting from the procedural system.
 */
USTRUCT()
struct FCosmicFoliageInstance
{
    GENERATED_BODY()

    /** Shared HISM to which the instance belongs. */
    FCosmicHISMKey HISMKey;

    /** Final transform of the instance */
    FTransform Transform;
};


/**
 * Asynchronous task responsible for generating foliage instances
 * for a planet CubeMap cell.
 *
 * Executes procedural generation based on noise, environmental
 * conditions, and vegetation layer distribution.
 */
class FFoliageGenerationTask : public FNonAbandonableTask
{
public:
    /** Generated foliage instance results */
    TArray<FCosmicFoliageInstance> ResultInstances;
    /** CubeMap cell to process */
    FCubeMapCell Cell;
    /** Foliage layer being generated */
    ECosmicFoliageLayer Layer;

    /**
     * Represents a sample point used during foliage generation.
     */
    struct FSeedPoint
    {
        FVector Direction = FVector::UpVector;      // Direction from planet center
        FVector WorldPosition = FVector::ZeroVector; // Position relative to planet
        FVector CachedNormal = FVector::UpVector;
        int32 AllocationIndex = INDEX_NONE;
        float Temperature = 0.0f;
        float Humidity = 0.0f;
        float Height = 0.0f;
        float Slope = 0.0f;
    }; 
    /**
     * Foliage generation task constructor.
     */
    FFoliageGenerationTask(
        const FCubeMapCell& InCell,
        ECosmicFoliageLayer InLayer,
        TSharedPtr<const TArray<FCosmicFoliageCollectionEntry>, ESPMode::ThreadSafe> InFoliageEntries,
        double InPlanetRadius,
        TSharedPtr<ICosmicNoiseStrategy> InNoiseGenerationStrategy,
        int32 InMaxInstancesPerCell,
        float InNormalSampleDistanceCm)
        : Cell(InCell)
        , Layer(InLayer)
        , FoliageEntries(MoveTemp(InFoliageEntries))
        , PlanetRadius(InPlanetRadius)
        , NoiseGenerationStrategy(InNoiseGenerationStrategy)
        , MaxInstancesPerCell(InMaxInstancesPerCell)
        , NormalSampleDistanceCm(InNormalSampleDistanceCm)
    {
    }

    /** Stat identifier for the threading system */
    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(FFoliageGenerationTask, STATGROUP_ThreadPoolAsyncTasks);
    }
    /** Main task execution */
    void DoWork();

private:

    /** Immutable snapshot created once on the game thread and shared across tasks. */
    TSharedPtr<const TArray<FCosmicFoliageCollectionEntry>, ESPMode::ThreadSafe> FoliageEntries;

    /** Planet radius */
    double PlanetRadius;

    /** Noise generation strategy */
    TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy;

    /** Cell area in km² */
    double CellAreaKm2 = 0.0;

    /** Safety and sampling limits configured by the spawner. */
    int32 MaxInstancesPerCell;
    float NormalSampleDistanceCm;

    /** Points generated for evaluation */
    TArray<FSeedPoint> SeedPoints;

    struct FMeshAllocation
    {
        int32 EntryIndex = INDEX_NONE;
        int32 MeshIndex = INDEX_NONE;
        int32 TargetCount = 0;
        bool bNeedsSurfaceNormal = true;
    };

    TArray<FMeshAllocation> Allocations;

    /** Calculates the actual spherical quadrilateral area of the cell. */
    double CalculateCellAreaKm2() const;

    /** Precalculates deterministic quotas, including stochastic rounding. */
    int32 PrepareAllocations(FRandomStream& Random);

    /**
     * Generates seed points within the cell.
     */
    void GenerateSeedPoints(FRandomStream& Random);

    /**
     * Evaluates environmental conditions (temperature, humidity, height, etc).
     */
    void EvaluateEnvironmentalConditions();

    /**
     * Creates final foliage instances from seed points.
     */
    void CreateFoliageInstances(FRandomStream& Random);

    /**
     * Calculates terrain slope and normal at a point.
     */
    void CalculateSlopeAndNormal(
        const FVector& Direction,
        float CenterHeight,
        float& OutSlope,
        FVector& OutNormal);
};
