// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "CosmicNoiseGenerationTask.h"
#include "CosmicMeshComponent.generated.h"

class ICosmicNoiseStrategy;

/**
 * Defines the logical quadrant used by the clipmap
 * to manage scrolling and reconstructions.
 */
UENUM(BlueprintType)
enum class EClipmapQuadrant : uint8
{
    /** Initial base position */
    TopLeft = 0,

    /** Offset to the right */ 
    TopRight = 1,

    /** Offset downwards */
    BottomLeft = 2,

    /** Offset to the right and downwards */
    BottomRight = 3
};

/**
 * Procedural component responsible for representing
 * an individual level of the clipmap system.
 *
 * Main features:
 * - Procedural mesh generation projected onto sphere.
 * - Generation of simplified spherical meshes.
 * - Asynchronous updating using noise tasks.
 * - Dynamic level rescaling.
 * - Visibility and transformation management.
 */
UCLASS()
class UCosmicMeshComponent : public UProceduralMeshComponent
{
    GENERATED_BODY()

public:

    /** Level index within clipmap */
    int32 LevelIndex;

    /** Grid resolution */
    int32 Resolution;

    /** Vertex spacing */
    int64 GridSpacing;

    /** Planet radius */
    double PlanetRadius;

    /** Indicates whether level is an outer ring */
    bool bIsRing;

    /** Indicates whether mesh represents a planet */
    bool bIsPlanet;

    /** Indicates whether procedural mesh has already been created */
    bool bMeshCreated = false;

    /** Indicates whether mesh is a full sphere */
    bool bIsSphereMesh = false;

    /** Indicates whether mesh is currently visible */
    bool bActiveMesh;

    /** Current patch transform */
    FTransform PatchTransform;

    /**
     * Base vertices deformed only on the sphere
     * without applying additional noise heights.
     */
    TArray<FVector> BaseVertices;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);

    virtual void OnComponentDestroyed(bool bDestroyingHierarchy);

    /**
     * Builds base procedural mesh projected
     * onto planet surface.
     */
    void BuildBaseProjectedMesh();

    /**
     * Builds a simplified full sphere.
     */
    void BuildSphereMesh();

    /**
     * Rescales current level using new spacing.
     *
     * @param GridSpacing New vertex spacing.
     */
    void ReScaleLevel(int64 GridSpacing);

    /**
     * Updates patch position and rotation.
     *
     * @param SurfacePos Position on surface.
     * @param PatchRotation Rotation aligned with normal.
     */
    void SetPositionAndRotation(const FVector& SurfacePos, const FRotator& PatchRotation);

    /**
     * Configures absolute projection of the level within a quantized
     * tangent frame. Center is expressed in this level's own cells.
     */
    void ConfigurePlanetaryProjection(
        const FTransform& InProjectionFrame,
        const FIntPoint& InGridCenter,
        uint64 InProjectionRevision,
        bool bInHasCoarserLevel);

    /** Indicates whether configured center or frame have not yet been generated. */
    bool IsPlanetaryProjectionUpdateRequired() const;

    /**
     * Visually enables or disables mesh.
     *
     * @param active Visibility state.
     */
    void SetMeshActive(bool active);

    /**
     * Requests an asynchronous procedural noise update.
     *
     * @param NoiseGenerationStrategy Active noise strategy.
     */
    void RequestMeshUpdate(TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy);

    /**
     * Checks whether generation task has finished
     * and applies newly generated geometry.
     *
     * @return True if mesh is already updated.
     */
    bool CheckAndApplyMeshUpdate();

    /**
     * Checks whether an active asynchronous task exists.
     *
     * @return True if task is still executing.
     */
    bool IsTaskActive();

    /**
     * Cancels any active asynchronous task.
     */
    void CancelAsyncWork();

protected:

    /** Asynchronous task used to generate procedural noise */
    FAsyncTask<FCosmicNoiseGenerationTask>* NoiseTask = nullptr;

    /** Indicates whether noise is currently being generated */
    bool bIsGeneratingNoise = false;

    /** State of incremental planetary geometry clipmap route. */
    bool bUseSnappedPlanetProjection = false;
    bool bHasCoarserLevel = false;
    FTransform ProjectionFrame = FTransform::Identity;
    FIntPoint RequestedGridCenter = FIntPoint::ZeroValue;
    uint64 RequestedProjectionRevision = 0;

    /** State of cache currently applied to this level. */
    FIntPoint CachedGridCenter = FIntPoint::ZeroValue;
    uint64 CachedProjectionRevision = MAX_uint64;
    TArray<float> CachedHeights;
    TArray<FLinearColor> CachedColors;
};
