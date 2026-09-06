// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "Interfaces/Interface_CollisionDataProvider.h"
#include "PhysicsEngine/BodySetup.h"
#include "CosmicCollisionComponent.generated.h"

class ICosmicNoiseStrategy;

/**
 * Component responsible for generating and updating procedural collision
 * used on the planetary surface.
 *
 * Implements a dynamic collision data provider compatible
 * with Chaos/Physics using procedurally generated triangles.
 *
 * Main features:
 * - Base collision mesh generation.
 * - Dynamic updating with procedural noise.
 * - Synchronous and asynchronous cooking.
 * - Debug collision visualization.
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

    /**
     * Default component constructor.
     */
    UCosmicCollisionComponent();

    /** Size of each triangle used for collision */
    UPROPERTY(EditAnywhere, Category = "Collision")
    float CollisionTriangleSize = 250.f;

    /** Collision grid resolution */
    UPROPERTY(EditAnywhere, Category = "Collision")
    int32 CollisionResolution = 12;

    /** Maximum distance at which collision is generated */
    UPROPERTY(EditAnywhere, Category = "Collision")
    double MaxCollisionDistance = 30000.f;

    /** Show collision mesh in editor for debugging */
    UPROPERTY(EditAnywhere, Category = "Collision")
    bool bShowCollisionMesh = false;

    /** Color used to visualize collision mesh */
    UPROPERTY(EditAnywhere, Category = "Collision", meta = (EditCondition = "bShowCollisionMesh"))
    FColor DebugColor = FColor::Green;

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
     * Generates the base collision mesh.
     *
     * @param Radius Planet radius.
     */
    void GenerateCollisionMesh(double Radius);

    /**
     * Updates collision vertices using procedural noise.
     *
     * @param NoiseGenerationStrategy Active noise strategy.
     * @param PlanetCenter Current planet center.
     */
    void UpdateCollisionMesh(TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy, const FVector& PlanetCenter);

    /**
     * Completely clears active collision.
     */
    void ClearCollision();

    /**
     * Indicates whether collision has already been built.
     *
     * @return True if valid collision exists.
     */
    bool IsBuilt() const;

protected:

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#if WITH_EDITOR

    /**
     * Executes automatically when properties are modified
     * from the editor details panel.
     */
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

#endif

    /** Collision data provider implementation */
    virtual bool GetPhysicsTriMeshData(FTriMeshCollisionData* CollisionData, bool InUseAllTriData) override;

    /** Indicates whether valid collision data exists */
    virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const override;

    /** Negative generation in X is not required */
    virtual bool WantsNegXTriMesh() override { return false; }

    /** Collision mesh size estimates */
    virtual bool GetTriMeshSizeEstimates(FTriMeshCollisionDataEstimates& OutTriMeshEstimates, bool bInUseAllTriData) const override;

    /** Gets the BodySetup used by physics */
    virtual UBodySetup* GetBodySetup() override;

private:

    /**
     * Builds or rebuilds physics collision.
     */
    void BuildCollision();

    /**
     * Updates only collision vertices.
     */
    void UpdateCollisionVertices();

    /** Main BodySetup used by the component */
    UPROPERTY(Transient)
    UBodySetup* BodySetup = nullptr;

    /** Queue of BodySetups used for asynchronous cooking */
    UPROPERTY()
    TArray<UBodySetup*> AsyncBodySetupQueue;

    /** Current collision center */
    FVector CurrentCollisionCenter;

    /** Current collision radius */
    float CurrentCollisionRadius = 0;

    /** Planet radius */
    double PlanetRadius = 0;

    /** Indicates whether collision rebuild is required */
    bool bNeedsRebuild = false;

    /** Indicates whether collision is active */
    bool bIsActive = false;

    /** Base vertices without deformation */
    TArray<FVector> BaseVertices;

    /** Base normals used for deformation */
    TArray<FVector> BaseNormals;

    /** Final deformed vertices */
    TArray<FVector> Verts;

    /** Triangle indices */
    TArray<int32> Tris;

    /**
     * Draws collision debug mesh.
     */
    void DrawDebugCollisionMesh();

    /**
     * Creates a new auxiliary BodySetup.
     *
     * @return Configured new BodySetup.
     */
    UBodySetup* CreateBodySetupHelper();

    /**
     * Creates the procedural main BodySetup.
     */
    void CreateProcMeshBodySetup();

    /**
     * Callback executed when asynchronous cooking finishes.
     *
     * @param bSuccess True if cooking succeeded.
     * @param FinishedBodySetup Finished BodySetup.
     */
    void FinishPhysicsAsyncCook(bool bSuccess, UBodySetup* FinishedBodySetup);
};