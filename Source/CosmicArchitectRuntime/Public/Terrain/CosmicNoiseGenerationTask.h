// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"

class ICosmicNoiseStrategy;

/**
 * Optional data to generate a clipmap level on a stable tangent frame.
 * Heights and colors include a two-texel halo to reconstruct normals
 * and the transition to the next level without re-evaluating noise.
 */
struct FCosmicPlanetClipmapGenerationSettings
{
    bool bEnabled = false;
    bool bHasCoarserLevel = false;
    int32 GridResolution = 0;
    FTransform ProjectionFrame = FTransform::Identity;
    FIntPoint DesiredGridCenter = FIntPoint::ZeroValue;
    FIntPoint PreviousGridCenter = FIntPoint::ZeroValue;
    uint64 ProjectionRevision = 0;
    uint64 PreviousProjectionRevision = MAX_uint64;
    TArray<float> CachedHeights;
    TArray<FLinearColor> CachedColors;
};

/**
 * Asynchronous task responsible for generating noise deformations,
 * normals, and colors for a procedural mesh.
 *
 * This task runs in the background using Unreal Engine's
 * async task system to avoid blocking the main thread
 * during terrain generation.
 */
class COSMICARCHITECTRUNTIME_API FCosmicNoiseGenerationTask : public FNonAbandonableTask
{
public:

    /** 
     * Reference to base mesh vertices.
     *
     * These vertices represent original geometry before
     * applying noise displacements.
     */
    const TArray<FVector>& BaseVertices;

    /**
     * Vertices calculated after applying procedural noise.
     */
    TArray<FVector> CalculatedVertices;

    /**
     * Normals calculated for the final geometry.
     */
    TArray<FVector> CalculatedNormals;

    /**
     * Colors calculated for each vertex.
     */
    TArray<FLinearColor> CalculatedColors;

    /** Scrollable cache returned to component upon task completion. */
    TArray<float> CalculatedHeightCache;
    TArray<FLinearColor> CalculatedColorCache;

    /** Center and revision to which clipmap results belong. */
    FIntPoint CalculatedGridCenter = FIntPoint::ZeroValue;
    uint64 CalculatedProjectionRevision = 0;

    /**
     * Owning component transform.
     */
    FTransform ComponentTransform;

    /**
     * Global planet center.
     */
    FVector PlanetCenter;

    /**
     * Planet radius used for spherical projections.
     */
    double PlanetRadius;

    /**
     * Grid vertex spacing.
     */
    double GridSpacing;

    /**
     * Indicates whether mesh represents a planet.
     */
    bool IsPlanet;

    /**
     * Indicates whether mesh corresponds to a full sphere.
     */
    bool IsSphere;

    /**
     * Noise strategy used to evaluate heights and colors.
     */
    TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy;

    /** Configuration of incremental planetary clipmap route. */
    FCosmicPlanetClipmapGenerationSettings ClipmapSettings;

    /**
     * Constructor for procedural generation asynchronous task.
     *
     * @param InBaseVerts Base mesh vertices.
     * @param InTransform Component transform.
     * @param InPlanetCenter Global planet center.
     * @param InPlanetRadius Planet radius.
     * @param InGridSpacing Vertex spacing.
     * @param InPlanet Indicates if mesh is planetary.
     * @param InIsSphere Indicates if geometry is a sphere.
     * @param InNoiseGenerationStrategy Noise strategy used.
     */
    FCosmicNoiseGenerationTask(
        const TArray<FVector>& InBaseVerts,
        FTransform InTransform,
        FVector InPlanetCenter,
        double InPlanetRadius,
        double InGridSpacing,
        bool InPlanet,
        bool InIsSphere,
        TSharedPtr<ICosmicNoiseStrategy> InNoiseGenerationStrategy,
        FCosmicPlanetClipmapGenerationSettings InClipmapSettings = {}
    );

    /**
     * Returns execution stats for profiling.
     */
    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(FCosmicArchitectNoiseGenerator, STATGROUP_ThreadPoolAsyncTasks);
    }

    /**
     * Executes procedural computation of vertices, normals, and colors.
     */
    void DoWork();

private:

    /** Generates or scrolls clipmap cache and reconstructs only its outputs. */
    void DoSnappedPlanetClipmapWork();
};
