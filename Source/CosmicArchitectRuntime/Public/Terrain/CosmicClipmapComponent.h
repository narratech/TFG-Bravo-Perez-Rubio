// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "CosmicClipmapComponent.generated.h"

class ICosmicNoiseStrategy;
class UCosmicMeshComponent;
class UCosmicFoliageSpawner;
class UCosmicCollisionComponent;
class UCosmicNoiseClass;

/**
 * Component responsible for managing the planetary clipmap system.
 *
 * Manages creation, updating, and destruction of dynamic levels of detail
 * around the player, including:
 * - Procedural mesh generation.
 * - Transitions between normal and performance mode.
 * - Near collision updating.
 * - Foliage generation.
 * - Dynamic planet materials.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent),
    HideCategories = (Activation, Tags, AssetUserData, Navigation, Rendering, Replication, Input, Actor, Collision, Cooking))
    class UCosmicClipmapComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    /**
     * Default component constructor.
     */
    UCosmicClipmapComponent();

    /**
     * Creates all levels of the clipmap system.
     */
    void CreateLevels();

    /**
     * Creates simplified level used in performance mode.
     *
     * @param bActive Indicates if level should start active.
     */
    void CreatePerformanceLevel(bool bActive);

    /**
     * Removes and destroys all generated levels.
     */
    void ClearLevels();

    /**
     * Clears inherited references after duplication without destroying original actor components.
     *
     * @param NewRoot Root component of new actor.
     */
    void ResetPointersAfterDuplicate(USceneComponent* NewRoot);

    /**
     * Configures visual parameters of planetary material.
     *
     * @param Color1 Main base color.
     * @param Color2 Secondary base color.
     * @param ColorCold Color for cold zones.
     * @param ColorHot Color for hot zones.
     * @param ColorSlope Color applied to slopes.
     * @param ScaleL Large noise scale.
     * @param ScaleM Medium noise scale.
     * @param ScaleS Small noise scale.
     */
    void SetMaterialData(FColor Color1, FColor Color2, FColor ColorCold, FColor ColorHot,
        FColor ColorSlope, float ScaleL, float ScaleM, float ScaleS);

    /**
     * Requests complete regeneration of meshes.
     */
    void RequestCompleteMeshUpdate();

    /**
     * Updates active noise generation strategy.
     */
    void UpdateNoiseEvaluator();

    /** Root to which generated levels are attached */
    USceneComponent* ParentRoot;

    /** Class responsible for generating procedural noise strategy */
    UCosmicNoiseClass* NoiseClass;

    /** Planet radius */
    double PlanetRadius;

    /** Base material used to generate dynamic instance */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* BaseMaterial;

    /** Default texture used by material */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UTexture2D* DefaultTexture;

    /** Base resolution of each clipmap level */
    UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "8", ClampMax = "256"))
    int32 BaseResolution = 128;

    /** Total number of clipmap levels */
    UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "1", ClampMax = "10"))
    int32 NumLevels = 4;

    /** Minimum allowed size for triangles */
    UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "10"))
    int32 MinTriangleSize = 100;

    /** Current base grid spacing */
    UPROPERTY(VisibleAnywhere, Category = "Clipmap")
    int64 BaseGridSpacing = 200;

    /** Altitude above which performance mode is activated */
    UPROPERTY(EditAnywhere, Category = "Clipmap")
    float HeightVisibility = 5.0f;

    /** Enables or disables clipmap system */
    UPROPERTY(EditAnywhere, Category = "Clipmap")
    bool UseClipmap = true;

    /** Freezes dynamic level generation */
    UPROPERTY(EditAnywhere, Category = "Clipmap")
    bool FreezeGeneration = false;

    /**
     * Angular step of planetary tangent frame. Within the same frame
     * levels only scroll their caches; when crossing it a regeneration occurs.
     */
    UPROPERTY(EditAnywhere, Category = "Clipmap|Spherical",
        meta = (ClampMin = "0.1", ClampMax = "45.0", UIMin = "0.5", UIMax = "15.0"))
    float PlanetGridSnapAngleDegrees = 5.0f;

    /** Component responsible for dynamic collision */
    UCosmicCollisionComponent* CollisionComponent;

    /** Component responsible for procedural foliage */
    UCosmicFoliageSpawner* FoliageSpawnerComponent;

protected:

    /** Active levels of the clipmap system */
    TArray<UCosmicMeshComponent*> Levels;

    /** Simplified level used in performance mode */
    UCosmicMeshComponent* FarLevel;

    /** Active procedural generation strategy */
    TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy;

    /**
     * Update phases distributed across frames
     * to reduce per-tick cost.
     */
    enum class EUpdatePhase : uint8
    {
        Foliage,
        Collision,
        Mesh
    };

    /** Time accumulated since last update */
    float ElapsedTime = 0;

    /** Currently used refresh time */
    float TimeToRefreshActive;

    /** Indicates whether system is in performance mode */
    bool bPerformaceMode = false;

    /** Indicates whether normal levels have been initialized */
    bool bInit = false;

    /** Indicates whether performance level has already been generated */
    bool bPerformanceBuild = false;

    /** Indicates whether active pending tasks exist */
    bool bPendingTasksRemaining = false;

    /** Waiting for transition to normal mode */
    bool bWaitingForNormalTransition = false;

    /** Waiting for transition to performance mode */
    bool bWaitingForPerformanceTransition = false;

    /** Indicates whether levels are currently being built */
    bool bBuildingLevels = false;

    /** Indicates whether system represents a spherical planet */
    bool IsPlanet = true;

    /** Original base spacing */
    int64 BaseSpacing = 200;

    /** Main planet color */
    FColor PlanetMainColor1 = FColor::Green;

    /** Secondary planet color */
    FColor PlanetMainColor2 = FColor::Red;

    /** Color for cold zones */
    FColor PlanetColdColor = FColor::Yellow;

    /** Color for hot zones */
    FColor PlanetHotColor = FColor::Yellow;

    /** Color used on slopes */
    FColor PlanetSlopeColor = FColor::Yellow;

    /** Small noise scale */
    float NoiseScaleSmall = 1.f;

    /** Medium noise scale */
    float NoiseScaleMedium = 1.f;

    /** Large noise scale */
    float NoiseScaleLarge = 1.f;

    /** System update interval */
    float TimeToRefresh = 0.01f;

    /** Last known player position */
    FVector LastPlayerPos;

    /** Last position used to update collision */
    FVector LastMeshPlayerPos;

    /** Current position of owning actor */
    FVector CurrentActorPosition;

    /** Accumulated delta in planar mode */
    FVector AccumulatedDelta = FVector::ZeroVector;

    /** Total accumulated clipmap shift */
    FIntPoint TotalShift = FIntPoint::ZeroValue;

    /** Current update phase */
    EUpdatePhase CurrentPhase = EUpdatePhase::Mesh;

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#if WITH_EDITOR

    /**
     * Executes automatically when properties are modified
     * from the editor details panel.
     */
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

#endif

    /**
     * Updates procedural foliage system.
     *
     * @param DeltaTime Elapsed time since last frame.
     * @param ViewerPos Current player position.
     * @param DistanceToSurface Distance to surface.
     */
    void UpdateFoliagePhase(float DeltaTime, const FVector& ViewerPos, float DistanceToSurface);

    /**
     * Updates near collision around player.
     *
     * @return True if collision was updated.
     */
    bool UpdateCollisionPhase(const FVector& ViewerPos, const FVector& SurfacePos,
        const FVector& N, float DistanceToSurface);

    /**
     * Updates clipmap levels.
     */
    void UpdateMeshPhase(const FVector& ViewerPos, const FVector& SurfacePos,
        const FVector& N, float DistanceToSurface);

    /** Updates quantized tangent frame if observer changes angular cell. */
    bool UpdateSnappedProjectionFrame(const FVector& ViewerNormal);

    /** Projects a sphere direction onto active absolute tangent plane. */
    FVector2D ProjectDirectionToSnappedFrame(const FVector& Direction) const;

    /** Configures all levels with integer centers aligned across LODs. */
    bool ConfigureLevelsForViewer(const FVector& ViewerNormal);

    /**
     * Generates or updates near collision around player.
     */
    bool UpdateCollisionNearPlayer(const FVector& SurfacePos, const FVector& SurfaceNormal, const double DistanceToSurface);

    /**
     * Builds dynamic instance of planetary material.
     */
    void BuildDynamicMaterial();

    /**
     * Calculates correct patch rotation
     * relative to surface normal.
     */
    FRotator GetPatchRotation(const FVector& SurfacePos) const;

    /**
     * Calculates true distance to surface using noise.
     */
    double GetDistanceToSurface(FVector& ViewerPos, FVector& SurfacePos, FVector& N);

    /**
     * Calculates approximate distance to surface without noise.
     */
    double GetFastDistanceToSurface(FVector& ViewerPos, FVector& SurfacePos, FVector& N);

    /**
     * Calculates distance to a planar surface.
     */
    float GetDistanceToPlainSurface(FVector& ViewerPos, FVector& SurfacePos, FVector& N);

    /**
     * Gets current player or camera position.
     */
    FVector GetPlayerLocation();

    /**
     * Calculates grid offset in planar mode.
     */
    FIntPoint ComputeGridShiftPlanar(const FVector& PlayerPos, float GridSpacing);

    /**
     * Calculates grid offset on spherical surface.
     */
    FIntPoint ComputeGridShiftSpherical(const FVector& PlayerPos, const FVector& CurrentSurfacePos, int64 GridSpacing);

    /**
     * Calculates grid offset according to surface type.
     */
    FIntPoint ComputeGridShift(const FVector& PlayerPos, const FVector& CurrentSurfacePos, float GridSpacing);

    /**
     * Gets spherical angles for a surface position.
     */
    FVector2D GetSurfaceAngles(const FVector& SurfacePos);

    /**
     * Calculates how many levels should be decreased.
     */
    int32 CalculateDecreaseSteps(const double DistanceToSurface) const;

    /**
     * Calculates how many levels should be increased.
     */
    int32 CalculateIncreaseSteps(const double DistanceToSurface) const;

    /**
     * Checks if a clipmap ring is visible.
     */
    bool IsClipmapRingVisible(const int32 LevelIndex, const double DistanceToSurface) const;

    /**
     * Checks if a clipmap ring is visible using manual spacing.
     */
    bool IsClipmapRingVisible(const int64 GridSpacing, const int64 Resolution, const double DistanceToSurface) const;

    /**
     * Decreases overall clipmap detail.
     */
    void DecreaseClipmapLevelFull(int32 Steps = 1);

    /**
     * Increases overall clipmap detail.
     */
    void IncreaseClipmapLevelFull(int32 Steps = 1);


private:

    /** Dynamic material used by planet */
    UPROPERTY(Transient, DuplicateTransient)
    UMaterialInstanceDynamic* DynamicPlanetMat;

    /** Last known position on surface */
    FVector PreviousSurfacePos = FVector::ZeroVector;

    /** Last recorded spherical angles of player */
    FVector2D LastSurfaceAngles;

    /** Accumulated linear delta on surface */
    FVector2D AccumulatedLinearDelta;

    /** Fixed tangent frame and angular cell that originated it. */
    FTransform SnappedProjectionFrame = FTransform::Identity;
    FIntPoint SnappedProjectionKey = FIntPoint::ZeroValue;
    bool bSnappedProjectionValid = false;
    uint64 SnappedProjectionRevision = 0;

    /** Common center expressed in coarsest level cells. */
    FIntPoint CoarsestGridCenter = FIntPoint::ZeroValue;
    bool bCoarsestGridCenterValid = false;
};
