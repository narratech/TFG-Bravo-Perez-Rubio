// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CosmicOctree.h"
#include "CosmicFoliageGenerationTask.h"
#include "CosmicFoliageSpawner.generated.h"

class UCosmicFoliageCollection;
class ICosmicNoiseStrategy;
 
/**
 * Component responsible for managing foliage generation and streaming
 * around the player using a CubeMap + Octree system.
 *
 * Controls:
 * - Cell activation/deactivation by distance.
 * - Execution of asynchronous generation tasks.
 * - Instance application via HISM.
 * - Vegetation layer management.
 */ 
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), 
    HideCategories = (Rendering, Lighting, Navigation, Replication, Physics,Collision,
        Activation, AssetUserData, HLOD, Cooking, Tags, ComponentReplication))
class COSMICARCHITECTFOLIAGE_API UCosmicFoliageSpawner : public UActorComponent
{
	GENERATED_BODY()

public:
    /**
     * Default constructor.
     */
    UCosmicFoliageSpawner();

    /**
     * Initializes the foliage spawning system.
     *
     * @param PlanetRadius Planet radius.
     */
    void InitFoliageSpawner(float PlanetRadius);

    /**
     * Updates the foliage system based on player position.
     *
     * @param DeltaTime Time between frames.
     * @param ViewerLocation Viewer position.
     * @param PlanetCenter Planet center.
     * @param PlanetRadius Planet radius.
     * @param DistanceToSurface Distance to surface.
     * @param NoiseGenerationStrategy Environmental noise strategy.
     */
    void UpdateFoliageSpawner(float DeltaTime, const FVector& ViewerLocation, const FVector& PlanetCenter, double PlanetRadius, double DistanceToSurface, TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy);

    /**
     * Cancels all active asynchronous tasks.
     */
    void CancelAsyncWork();

    /**
     * Clears all generated foliage instances.
     */
    void ClearFoliage();

    /** Foliage collection used for generation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage")
    UCosmicFoliageCollection* FoliageCollection;

    /** Near layer activation radius */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage", meta = (ClampMin = "0.02"))
    float NearLayerRadiusKm = 0.05f;

    /** Medium layer activation radius */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage", meta = (ClampMin = "0.02"))
    float MediumLayerRadiusKm = 0.2f;

    /** Far layer activation radius */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage", meta = (ClampMin = "0.02"))
    float FarLayerRadiusKm = 0.5f;

    /** Maximum instances applied to the world per frame */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Performance", meta = (ClampMin = "1"))
    int32 MaxInstancesGeneratedPerFrame = 100;

    /** Maximum number of cells computed simultaneously in the thread pool. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Performance", meta = (ClampMin = "1", ClampMax = "64"))
    int32 MaxConcurrentGenerationTasks = 16;

    /** Safety limit to prevent cells configured with millions of instances. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Performance", meta = (ClampMin = "1", ClampMax = "100000"))
    int32 MaxInstancesPerCell = 1000;

    /** Distance used to estimate the procedural terrain normal. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Performance", meta = (ClampMin = "1.0"))
    float NormalSampleDistanceCm = 500.0f;

    /** Fraction of radius the observer must move before re-querying the octree. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Performance", meta = (ClampMin = "0.001", ClampMax = "0.25"))
    float VisibilityUpdateDistanceRatio = 0.02f;

    /** Priority order to generate, apply and remove layers (default: 1.Far 2.Medium 3.Near). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Performance")
    TArray<ECosmicFoliageLayer> FoliageLayerPriority;

protected:
    virtual void BeginDestroy() override;
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
#if WITH_EDITOR
    virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    /**
     * Octree responsible for spatial subdivision of the planet.
     */
    FCosmicOctree Octree;

    /**
     * Active cells structure per foliage layer.
     */
    UPROPERTY()
    FCosmicFoliageLayerCells LayerCells[3];

    void ProcessApplyQueue(const FVector& ViewerDir, int32& RemainingInstanceBudget);
    void ProcessDeactivationQueue(int32& RemainingInstanceBudget);
    void UpdateOctreeAndGenerate(const FVector& ViewerLocation, double DistanceToSurface, const FVector& PlanetCenter);
    void UpdateFoliageGeneration();
    void GenerateCellFoliage(const FCubeMapCell& Cell, double PlanetRadius, ECosmicFoliageLayer Layer, TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy);
    void StartQueuedGenerationTasks(const FVector& ViewerDir, double PlanetRadius, TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy);
    void ClearDelegates();

private:

    struct FPendingQueuedCell
    {
        FCubeMapCell Cell;
        FVector UnitDirection = FVector::UpVector;
    };

    struct FPendingApplyCell
    {
        FCubeMapCell Cell;
        ECosmicFoliageLayer Layer;
        FVector UnitDirection = FVector::UpVector;
        TArray<FCosmicFoliageInstance> Instances;
        int32 NextInstanceIndex = 0;
    };

    TArray<FPendingApplyCell> ApplyQueues[3];
    TArray<FCubeMapCell> PendingDeactivation[3];
    TArray<FPendingQueuedCell> QueuedCells[3];
    TSet<FCubeMapCell> PendingDeactivationCells[3];
    TSet<FCubeMapCell> CellsBeingDeactivated[3];

    TSet<FCubeMapCell> CurrentVisibleCells[3];

    UPROPERTY()
    TMap<FCosmicHISMKey, FCosmicSharedHISMData> SharedHISMs;

    TSet<FCubeMapCell> PendingCells[3];
    TArray<FAsyncTask<FFoliageGenerationTask>*> ActiveTasks[3];

    /** Lightweight cache to avoid traversing the octree when coverage cannot change. */
    FVector LastVisibilityQueryLocation[3];
    float LastVisibilityRadiusKm[3] = { 0.0f, 0.0f, 0.0f };
    bool bVisibilityQueryValid[3] = { false, false, false };
    bool bLayerWasEnabled[3] = { false, false, false };
    uint8 ConfiguredLayerMask = 0;
    bool bLayerMaskDirty = true;
    TSharedPtr<const TArray<FCosmicFoliageCollectionEntry>, ESPMode::ThreadSafe> FoliageEntriesSnapshot;

    float GetLayerRadius(ECosmicFoliageLayer Layer) const;
    int32 GetActiveTaskCount() const;
    void GetLayerPriorityIndices(int32 OutLayerIndices[3]) const;
    void RefreshConfiguredLayerMask();
    void CancelLayerAsyncWork(int32 LayerIndex);
    /** Resets queues and visibility cache for a layer. */
    void ResetLayerState(int32 LayerIndex);
    /** Cancels generation and removes instances of a single layer. */
    void ClearFoliageLayer(ECosmicFoliageLayer Layer);
    /** Applies generated instances to the world */
    void ApplyGeneratedInstances(const FCubeMapCell& Cell, ECosmicFoliageLayer Layer, TArrayView<const FCosmicFoliageInstance> Instances);

    FCosmicSharedHISMData* GetOrCreateSharedHISM(const FCosmicHISMKey& Key);
    int32 RemoveCellInstances(int32 LayerIndex, const FCubeMapCell& Cell, int32 InstanceBudget);

};
