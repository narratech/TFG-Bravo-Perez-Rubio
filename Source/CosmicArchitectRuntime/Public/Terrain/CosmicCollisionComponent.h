// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "Interfaces/Interface_CollisionDataProvider.h"
#include "PhysicsEngine/BodySetup.h"

class ICosmicNoiseStrategy;
class FCosmicCollisionGenerationTask;
template<typename TTask> class FAsyncTask;

#include "CosmicCollisionComponent.generated.h"

/**
 * Component responsible for generating and updating procedural collision
 * used on the planetary surface.
 *
 * Implements a double-buffering (ping-pong bodies) system with asynchronous
 * noise computation and asynchronous physics cooking to eliminate Game Thread hitches
 * and ensure seamless physical ground contact at all times.
 */


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent),
    HideCategories = (Rendering, Lighting, Navigation, Replication, Physics, LOD, TextureStreaming,
        Activation, AssetUserData, HLOD, Cooking, Tags, ComponentReplication, Mobile, RayTracing))
class COSMICARCHITECTRUNTIME_API UCosmicCollisionComponent :
    public UPrimitiveComponent,
    public IInterface_CollisionDataProvider
{
    GENERATED_BODY()

public:

    /** Default component constructor. */
    UCosmicCollisionComponent();

    /** Size of each triangle used for collision */
    UPROPERTY(EditAnywhere, Category = "Collision")
    float CollisionTriangleSize = 250.f;

    /** Collision grid resolution */
    UPROPERTY(EditAnywhere, Category = "Collision")
    int32 CollisionResolution = 16;

    /** Number of grid cells the player must travel before triggering a collision update */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision", meta = (ClampMin = "1"))
    int32 UpdateCellInterval = 4;

    /** Maximum distance at which collision is generated */
    UPROPERTY(EditAnywhere, Category = "Collision")
    double MaxCollisionDistance = 30000.f;

    /** Show collision mesh in editor for debugging */
    UPROPERTY(EditAnywhere, Category = "Collision")
    bool bShowCollisionMesh = false;

    /** Color used to visualize active collision mesh */
    UPROPERTY(EditAnywhere, Category = "Collision", meta = (EditCondition = "bShowCollisionMesh"))
    FColor DebugColor = FColor::Green;

    /** Color used to visualize in-flight standby collision mesh */
    UPROPERTY(EditAnywhere, Category = "Collision", meta = (EditCondition = "bShowCollisionMesh"))
    FColor StandbyDebugColor = FColor(255, 165, 0);

    /** Collision debug line thickness */
    UPROPERTY(EditAnywhere, Category = "Collision", meta = (EditCondition = "bShowCollisionMesh", ClampMin = "0"))
    float DebugLineWidth = 20.f;

    /** Use complex collision as simple collision */
    UPROPERTY(EditAnywhere, Category = "Collision")
    bool bUseComplexAsSimpleCollision = true;

    /** Use asynchronous cooking for physics */
    UPROPERTY(EditAnywhere, Category = "Collision")
    bool bUseAsyncCooking = true;

    /**
     * Forces a full collision rebuild.
     */
    UFUNCTION(CallInEditor, Category = "Collision")
    void RebuildCollision();

    /**
     * Generates the base collision grid geometry.
     *
     * @param Radius Planet radius.
     */
    void GenerateCollisionMesh(double Radius);

    /**
     * Requests an asynchronous collision update near the specified surface point.
     * Generates deformed vertices on background threads and cooks physics asynchronously
     * on the standby body before atomically swapping it with the active body.
     *
     * @param SurfacePos Target position on planet surface.
     * @param SurfaceNormal Outward surface normal at target position.
     * @param InPlanetRadius Planet radius.
     * @param NoiseGenerationStrategy Active procedural noise strategy.
     * @param PlanetCenter Current planet center.
     */
    void RequestCollisionUpdate(
        const FVector& SurfacePos,
        const FVector& SurfaceNormal,
        double InPlanetRadius,
        TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy,
        const FVector& PlanetCenter
    );

    /**
     * Updates collision vertices using procedural noise (Legacy / synchronous wrapper).
     *
     * @param NoiseGenerationStrategy Active noise strategy.
     * @param PlanetCenter Current planet center.
     */
    void UpdateCollisionMesh(TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy, const FVector& PlanetCenter);

    /**
     * Completely clears active collision on both bodies and aborts ongoing tasks.
     */
    void ClearCollision();

    /**
     * Indicates whether valid collision exists.
     *
     * @return True if valid active collision exists.
     */
    bool IsBuilt() const;

    /**
     * Computes tangent patch rotation from an outward surface normal.
     *
     * @param Normal Outward surface normal.
     * @return Rotator aligned to the tangent plane.
     */
    static FRotator ComputePatchRotation(const FVector& Normal);

    /**
     * Returns the movement threshold in world units (cm) required to trigger a collision update.
     *
     * @return Distance in cm (CollisionTriangleSize * UpdateCellInterval).
     */
    UFUNCTION(BlueprintPure, Category = "Collision")
    float GetUpdateDistanceThreshold() const { return CollisionTriangleSize * static_cast<float>(FMath::Max(1, UpdateCellInterval)); }

protected:

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    /** Collision data provider implementation */
    virtual bool GetPhysicsTriMeshData(FTriMeshCollisionData* CollisionData, bool InUseAllTriData) override;
    virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const override;
    virtual bool WantsNegXTriMesh() override { return false; }
    virtual bool GetTriMeshSizeEstimates(FTriMeshCollisionDataEstimates& OutTriMeshEstimates, bool bInUseAllTriData) const override;
    virtual UBodySetup* GetBodySetup() override;

private:

    enum class ECollisionUpdateState : uint8
    {
        Idle,
        NoiseTaskRunning,
        PhysicsCooking
    };

    /** Indicates whether this instance is the secondary companion body */
    UPROPERTY()
    bool bIsCompanion = false;

    /** Companion ping-pong collision component */
    UPROPERTY(Transient)
    UCosmicCollisionComponent* CompanionPatch = nullptr;

    /** Indicates whether this primary component is currently the active physical body */
    bool bPrimaryIsActiveBody = true;

    /** Current update state machine state */
    ECollisionUpdateState UpdateState = ECollisionUpdateState::Idle;

    /** Background noise calculation task */
    FAsyncTask<FCosmicCollisionGenerationTask>* NoiseTask = nullptr;

    /** Main BodySetup used by this component */
    UPROPERTY(Transient)
    UBodySetup* BodySetup = nullptr;

    /** Queue of BodySetups used for asynchronous cooking */
    UPROPERTY()
    TArray<UBodySetup*> AsyncBodySetupQueue;

    /** Base vertices without deformation */
    TArray<FVector> BaseVertices;

    /** Base normals used for deformation */
    TArray<FVector> BaseNormals;

    /** Final deformed vertices */
    TArray<FVector> Verts;

    /** Triangle indices */
    TArray<int32> Tris;

    /** Planet radius */
    double PlanetRadius = 0;

    /** Indicates whether base geometry has been generated */
    bool bBaseGeometryGenerated = false;

    /** Indicates whether collision is active */
    bool bIsActive = false;

    /** Indicates whether collision rebuild is required */
    bool bNeedsRebuild = false;

    /** Target transform for the pending standby patch */
    FTransform PendingTransform;

    /** Last update location to prevent redundant triggers */
    FVector LastUpdatedLocation = FVector(MAX_flt);

    /** Queued update request parameters for fast movement */
    bool bHasQueuedUpdate = false;
    FVector QueuedSurfacePos;
    FVector QueuedSurfaceNormal;
    double QueuedPlanetRadius = 0;
    TSharedPtr<ICosmicNoiseStrategy> QueuedNoiseStrategy;
    FVector QueuedPlanetCenter;

    /** Internal cook completion callback */
    TFunction<void(bool)> PatchCookFinishedCallback;

    /** Ensures the companion ping-pong component is created and initialized */
    void EnsureCompanionCreated();

    /** Builds base planar-to-sphere grid data */
    void BuildBaseGrid(double Radius);

    /** Starts cooking a patch with given vertices and transform */
    void StartPatchCook(TArray<FVector>&& InVerts, const FTransform& InTransform, bool bAsync, TFunction<void(bool)> OnComplete);

    /** Creates a new auxiliary BodySetup */
    UBodySetup* CreateBodySetupHelper();

    /** Creates procedural BodySetup */
    void CreateProcMeshBodySetup();

    /** Callback executed when asynchronous cooking finishes */
    void FinishPhysicsAsyncCook(bool bSuccess, UBodySetup* FinishedBodySetup);

    /** Callback executed when the standby patch finishes cooking */
    void OnStandbyCookFinished(bool bSuccess);

    /** Activates physics collision on this component */
    void ActivatePhysics();

    /** Deactivates physics collision on this component */
    void DeactivatePhysics();

    /** Draws collision debug mesh */
    void DrawDebugCollisionMesh();
};