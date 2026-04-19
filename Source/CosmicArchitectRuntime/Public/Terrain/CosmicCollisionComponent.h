#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "Interfaces/Interface_CollisionDataProvider.h"
#include "PhysicsEngine/BodySetup.h"
#include "CosmicCollisionComponent.generated.h"

class UCosmicNoiseSettings;
class ICosmicNoiseStrategy;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class COSMICARCHITECTRUNTIME_API UCosmicCollisionComponent :
    public UPrimitiveComponent,
    public IInterface_CollisionDataProvider
{
    GENERATED_BODY()

public:

    UCosmicCollisionComponent();

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, Category = "Collision")
    float CollisionTriangleSize = 300.f;

    UPROPERTY(EditAnywhere, Category = "Collision")
    int32 CollisionResolution = 16;

    UPROPERTY(EditAnywhere, Category = "Collision")
    float MaxCollisionDistance = 20000.f;

    /** Mostrar malla de colision en el editor (para depuracion) */
    UPROPERTY(EditAnywhere, Category = "Collision")
    bool bShowCollisionMesh = false;

    /** Color de la malla de colision en el editor */
    UPROPERTY(EditAnywhere, Category = "Collision", meta = (EditCondition = "bShowCollisionMesh"))
    FColor DebugColor = FColor::Green;

    /** Transparencia de la malla de colision (0-1) */
    UPROPERTY(EditAnywhere, Category = "Collision", meta = (EditCondition = "bShowCollisionMesh", ClampMin = "0"))
    float DebugLineWidth = 20.f;

    UPROPERTY(EditAnywhere, Category = "Collision")
    bool bUseComplexAsSimpleCollision = true;

    UPROPERTY(EditAnywhere, Category = "Collision")
    bool bUseAsyncCooking = true;

    UFUNCTION(CallInEditor, Category = "Collision")
    void RebuildCollision();

    void GenerateCollisionMesh(double Radius);

    void UpdateCollisionMesh(TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy, const FVector& PlanetCenter);

    void ClearCollision();

    bool IsBuilt() const;

protected:

#if WITH_EDITOR
    // Se llama automáticamente cuando cambias algo en el panel de Detalles
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
    /** Collision provider interface */
    virtual bool GetPhysicsTriMeshData(FTriMeshCollisionData* CollisionData, bool InUseAllTriData) override;
    virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const override;
    virtual bool WantsNegXTriMesh() override { return false; }
    virtual bool GetTriMeshSizeEstimates(FTriMeshCollisionDataEstimates& OutTriMeshEstimates, bool bInUseAllTriData) const override;

    virtual UBodySetup* GetBodySetup() override;

private:

    void BuildCollision();

    void UpdateCollisionVertices();

    UPROPERTY(Transient)
    UBodySetup* BodySetup = nullptr;

    UPROPERTY()
    TArray<UBodySetup*> AsyncBodySetupQueue;

    FVector CurrentCollisionCenter;
    float CurrentCollisionRadius = 0;
    double PlanetRadius = 0;
    bool bNeedsRebuild = false;
    bool bIsActive = false;

    TArray<FVector> BaseVertices;
    TArray<FVector> BaseNormals;

    TArray<FVector> Verts;
    TArray<int32> Tris;

    void DrawDebugCollisionMesh();
    UBodySetup* CreateBodySetupHelper();
    void CreateProcMeshBodySetup();

    void FinishPhysicsAsyncCook(bool bSuccess, UBodySetup* FinishedBodySetup);
};