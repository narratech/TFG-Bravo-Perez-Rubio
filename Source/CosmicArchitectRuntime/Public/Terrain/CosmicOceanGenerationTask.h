// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"

/**
 * Settings required for the multi-level ocean clipmap generation task.
 */
struct FCosmicOceanClipmapSettings
{
    int32 NumLevels = 3;
    int32 Resolution = 128;
    int64 BaseGridSpacing = 200;
    double OceanRadius = 100000.0;
    FTransform PatchTransform = FTransform::Identity;
    FIntPoint CoarsestGridCenter = FIntPoint::ZeroValue;
    uint64 ProjectionRevision = 0;
};

/**
 * Asynchronous task responsible for calculating spherical vertex positions
 * and normals for all concentric LOD levels of the ocean in a single pass.
 *
 * Runs on Unreal Engine's thread pool without evaluating CPU noise.
 * Employs the identical projection geometry as UCosmicMeshComponent::BuildBaseProjectedMesh.
 */
class COSMICARCHITECTRUNTIME_API FCosmicOceanGenerationTask : public FNonAbandonableTask
{
public:

    /** Generation settings passed from the component. */
    FCosmicOceanClipmapSettings Settings;

    /** Vertices calculated after spherical projection. */
    TArray<FVector> CalculatedVertices;

    /** Normals calculated outward from the sphere center. */
    TArray<FVector> CalculatedNormals;

    /** Center and revision to which results belong. */
    FIntPoint CalculatedGridCenter = FIntPoint::ZeroValue;
    uint64 CalculatedProjectionRevision = 0;

    /**
     * Constructor for ocean mesh generation async task.
     */
    FCosmicOceanGenerationTask(FCosmicOceanClipmapSettings InSettings);

    /**
     * Returns execution stats for profiling.
     */
    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(FCosmicOceanGenerationTask, STATGROUP_ThreadPoolAsyncTasks);
    }

    /**
     * Executes procedural computation of spherical vertices and normals
     * for all concentric LOD levels.
     */
    void DoWork();
};
