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
class UCosmicOceanComponent;

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
     * @param InArchetypeIndex Preset archetype index (0: Earth, 1: Mars, 2: Ice Moon, 3: Volcanic).
     * @param bInUseCustomArchetype If true, enables custom terrain and rock colors.
     * @param InTerrainColorLow Color for lowland depressions/basins.
     * @param InTerrainColorMid Color for midland plains.
     * @param InTerrainColorHigh Color for highlands and peaks.
     * @param InRockColor Color for steep cliffs, slopes and bedrock.
     * @param bInEnableSnow If true, enables dynamic snow accumulation.
     */
    void SetMaterialData(
        int32 InArchetypeIndex,
        bool bInUseCustomArchetype,
        const FLinearColor& InTerrainColorLow,
        const FLinearColor& InTerrainColorMid,
        const FLinearColor& InTerrainColorHigh,
        const FLinearColor& InRockColor,
        bool bInEnableSnow
    );

    /**
     * Updates active DynamicPlanetMat with the component's current material properties.
     */
    void UpdateMaterialParameters();

    /**
     * Requests complete regeneration of meshes.
     */
    void RequestCompleteMeshUpdate();

    /**
     * Updates active noise generation strategy.
     */
    void UpdateNoiseEvaluator();

    /**
     * Updates clipmap levels.
     */
    void UpdateMeshPhase(const FVector& ViewerPos, const FVector& SurfacePos,
        const FVector& N, float DistanceToSurface);

    /**
     * Calculates true distance to surface using noise.
     */
    double GetDistanceToSurface(FVector& ViewerPos, FVector& SurfacePos, FVector& N);

    /**
     * Calculates approximate distance to surface without noise.
     */
    double GetFastDistanceToSurface(FVector& ViewerPos, FVector& SurfacePos, FVector& N);

    /**
     * Gets current player or camera position.
     */
    FVector GetPlayerLocation();

    /** Root to which generated levels are attached */
    USceneComponent* ParentRoot;

    /** Class responsible for generating procedural noise strategy */
    UCosmicNoiseClass* NoiseClass;

    /** Planet radius */
    double PlanetRadius;

    /** Base material used to generate dynamic instance */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* BaseMaterial;


    /** Archetype preset index (0: Earth-Like, 1: Mars-Like, 2: Ice Moon, 3: Volcanic). Used when bUseCustomArchetype is false. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Archetype", meta = (ClampMin = "0"))
    int32 ArchetypeIndex = 0;

    /** Enables dynamic snow accumulation on cold zones and mountain peaks. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Archetype")
    bool bEnableSnow = true;

    /** Steep cliffs, slopes, and bedrock strata color. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Colors")
    FLinearColor RockColor = FLinearColor(0.799f, 0.397f, 0.171f, 1.0f);

    /** Highlands / Ridges / Peaks terrain tint color. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Colors")
    FLinearColor TerrainColorHigh = FLinearColor(0.723f, 0.168f, 0.012f, 1.0f);

    /** Lowlands / Depressions / Basin terrain tint color. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Colors")
    FLinearColor TerrainColorLow = FLinearColor(0.212f, 0.028f, 0.026f, 1.0f);

    /** Midlands / Plains / Dominant terrain tint color. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Colors")
    FLinearColor TerrainColorMid = FLinearColor(0.509f, 0.014f, 0.008f, 1.0f);

    /** Controls whether custom palette colors (TerrainColorLow/Mid/High, RockColor) are used instead of preset archetypes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Archetype")
    bool bUseCustomArchetype = false;

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

    /** Component responsible for procedural foliage */
    UCosmicFoliageSpawner* FoliageSpawnerComponent;

    /** Component responsible for ocean clipmap and far sphere */
    UCosmicOceanComponent* OceanComponent = nullptr;

protected:

    /** Active levels of the clipmap system */
    TArray<UCosmicMeshComponent*> Levels;

    /** Simplified level used in performance mode */
    UCosmicMeshComponent* FarLevel;

    /** Active procedural generation strategy */
    TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy;

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


    /** Current position of owning actor */
    FVector CurrentActorPosition;

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);

#if WITH_EDITOR

    /**
     * Executes automatically when properties are modified
     * from the editor details panel.
     */
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

#endif

    /** Updates quantized tangent frame if observer changes angular cell. */
    bool UpdateSnappedProjectionFrame(const FVector& ViewerNormal);

    /** Projects a sphere direction onto active absolute tangent plane. */
    FVector2D ProjectDirectionToSnappedFrame(const FVector& Direction) const;

    /** Configures all levels with integer centers aligned across LODs. */
    bool ConfigureLevelsForViewer(const FVector& ViewerNormal);

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


    /** Fixed tangent frame and angular cell that originated it. */
    FTransform SnappedProjectionFrame = FTransform::Identity;
    FIntPoint SnappedProjectionKey = FIntPoint::ZeroValue;
    bool bSnappedProjectionValid = false;
    uint64 SnappedProjectionRevision = 0;

	/** Common center expressed in coarsest level cells. */
	FIntPoint CoarsestGridCenter = FIntPoint::ZeroValue;
	bool bCoarsestGridCenterValid = false;

	/** Last computed viewer coordinates in tangent projection plane. */
	FVector2D LastViewerCoordinates = FVector2D::ZeroVector;
};
