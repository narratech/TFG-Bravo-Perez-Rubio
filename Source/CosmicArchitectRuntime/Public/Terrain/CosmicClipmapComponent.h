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
     * Configures manual biome, strata, and environmental parameters of the planetary material
     * and pushes them to the active DynamicPlanetMat.
     */
    void SetMaterialData(
        const FLinearColor& InBiomeColorPolarIce,
        const FLinearColor& InBiomeColorTundra,
        const FLinearColor& InBiomeColorTaiga,
        const FLinearColor& InBiomeColorGrassland,
        const FLinearColor& InBiomeColorForest,
        const FLinearColor& InBiomeColorDesert,
        const FLinearColor& InBiomeColorSavannah,
        const FLinearColor& InBiomeColorRainforest,
        const FLinearColor& InStrataColor1,
        const FLinearColor& InStrataColor2,
        const FLinearColor& InStrataColor3,
        float InStrataFrequency,
        float InStrataDistortion,
        float InStrataNormalStrength,
        bool bInEnableSnow,
        const FLinearColor& InSnowColor,
        float InSnowTemperatureThreshold,
        float InWetnessIntensity,
        float InMagmaIntensity,
        float InMacroFrequency,
        float InMicroFrequency,
        float InNormalStrength,
        float InSlopeCliffThreshold,
        float InSlopeCliffFallOff
    );

    /**
     * Updates active DynamicPlanetMat with the component's current material properties.
     */
    UFUNCTION(BlueprintCallable, Category = "Materials")
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

    // --- Analytical Whittaker Biomes (Earth-Like defaults) ---

    /** Biome 0: Polar Ice / Glacial albedo color. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Biomes")
    FLinearColor BiomeColorPolarIce = FLinearColor(0.88f, 0.92f, 0.96f, 1.0f);

    /** Biome 1: Tundra albedo color. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Biomes")
    FLinearColor BiomeColorTundra = FLinearColor(0.42f, 0.40f, 0.32f, 1.0f);

    /** Biome 2: Taiga / Boreal Conifer albedo color. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Biomes")
    FLinearColor BiomeColorTaiga = FLinearColor(0.12f, 0.22f, 0.10f, 1.0f);

    /** Biome 3: Temperate Grassland albedo color. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Biomes")
    FLinearColor BiomeColorGrassland = FLinearColor(0.32f, 0.38f, 0.16f, 1.0f);

    /** Biome 4: Temperate Deciduous Forest albedo color. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Biomes")
    FLinearColor BiomeColorForest = FLinearColor(0.14f, 0.30f, 0.09f, 1.0f);

    /** Biome 5: Arid Sand Desert albedo color. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Biomes")
    FLinearColor BiomeColorDesert = FLinearColor(0.76f, 0.58f, 0.35f, 1.0f);

    /** Biome 6: Savannah / Scrubland albedo color. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Biomes")
    FLinearColor BiomeColorSavannah = FLinearColor(0.55f, 0.48f, 0.22f, 1.0f);

    /** Biome 7: Tropical Rainforest albedo color. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Biomes")
    FLinearColor BiomeColorRainforest = FLinearColor(0.06f, 0.20f, 0.05f, 1.0f);

    // --- Geological Sedimentary Strata (Earth-Like defaults) ---

    /** Sedimentary rock stratum tier 1 color (e.g. Sandstone). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Strata")
    FLinearColor StrataColor1 = FLinearColor(0.58f, 0.45f, 0.32f, 1.0f);

    /** Sedimentary rock stratum tier 2 color (e.g. Iron Clay). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Strata")
    FLinearColor StrataColor2 = FLinearColor(0.42f, 0.25f, 0.18f, 1.0f);

    /** Sedimentary rock stratum tier 3 color (e.g. Dark Shale). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Strata")
    FLinearColor StrataColor3 = FLinearColor(0.22f, 0.20f, 0.22f, 1.0f);

    /** Vertical repetition frequency of cliff rock strata bands. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Strata", meta = (ClampMin = "0.001", UIMin = "0.01", UIMax = "0.5"))
    float StrataFrequency = 0.08f;

    /** 3D noise folding / undulation intensity of cliff rock strata. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Strata", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0"))
    float StrataDistortion = 0.45f;

    /** Horizontal shelf / ledge normal perturbation strength on cliffs. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Strata", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0"))
    float StrataNormalStrength = 0.35f;

    // --- Snow, Wetness, Magma & Surface Relief (Earth-Like defaults) ---

    /** Enables dynamic snow accumulation on cold zones and mountain peaks. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Environment")
    bool bEnableSnow = true;

    /** Snow / glaciation albedo tint color. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Environment", meta = (EditCondition = "bEnableSnow"))
    FLinearColor SnowColor = FLinearColor(0.92f, 0.94f, 0.98f, 1.0f);

    /** Temperature threshold below which snow begins to accumulate [0..1]. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Environment", meta = (EditCondition = "bEnableSnow", ClampMin = "0.0", ClampMax = "1.0"))
    float SnowTemperatureThreshold = 0.22f;

    /** Intensity of porous soil darkening and water puddles in humid flat areas. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Environment", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0"))
    float WetnessIntensity = 1.0f;

    /** Volcanic thermal magma emission intensity (0 disables magma emission). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Environment", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "5.0"))
    float MagmaIntensity = 0.0f;

    /** Primary macro 3D volume noise frequency. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Surface", meta = (ClampMin = "0.0000001"))
    float MacroFrequency = 0.00003f;

    /** Secondary meso/micro 3D volume noise frequency. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Surface", meta = (ClampMin = "0.0000001"))
    float MicroFrequency = 0.000002f;

    /** Procedural tangent-space normal perturbation strength. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Surface", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "3.0"))
    float NormalStrength = 1.0f;

    /** Slope threshold where flat biome soil transitions into stratified cliff rock [0..1]. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Surface", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SlopeCliffThreshold = 0.15f;

    /** Smoothness falloff band around SlopeCliffThreshold. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Surface", meta = (ClampMin = "0.001", ClampMax = "1.0"))
    float SlopeCliffFallOff = 0.20f;

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
