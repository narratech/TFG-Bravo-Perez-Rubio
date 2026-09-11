// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.


#include "Terrain/CosmicMeshComponent.h"
#include "Terrain/CosmicClipmapGeometry.h"
#include "ICosmicNoiseStrategy.h"


void UCosmicMeshComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {

    CancelAsyncWork();
    Super::EndPlay(EndPlayReason);
}

void UCosmicMeshComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    CancelAsyncWork();
    Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UCosmicMeshComponent::BuildBaseProjectedMesh()
{
    const int32 VertRes = Resolution + 1;
    const int32 TotalVertices = VertRes * VertRes;
    const int32 HalfRes = Resolution / 2;

    ClearAllMeshSections();

    CachedHeights.Reset();
    CachedColors.Reset();
    CachedProjectionRevision = MAX_uint64;

    BaseVertices.Empty(TotalVertices);

    TArray<FVector> BaseNormals;
    BaseNormals.Reserve(TotalVertices);
     
    TArray<FVector2D> UVs;
    TArray<FProcMeshTangent> BaseTangents;

    UVs.Empty(TotalVertices);
    BaseTangents.Empty(TotalVertices);

    TArray<int32> Triangles;

    for (int32 y = 0; y < VertRes; ++y)
    {
        for (int32 x = 0; x < VertRes; ++x)
        {
            const double WorldX = (x - HalfRes) * GridSpacing;
            const double WorldY = (y - HalfRes) * GridSpacing;

            FVector BasePosition;
            FVector Normal;
            FVector TangentDir;
            FCosmicClipmapGeometry::ProjectPlanarGridPointToSphere(
                WorldX, WorldY, PlanetRadius, BasePosition, Normal, TangentDir);

            BaseVertices.Add(BasePosition);
            BaseNormals.Add(Normal);
            BaseTangents.Add(FProcMeshTangent(TangentDir.X, TangentDir.Y, TangentDir.Z));

            // UVs
            UVs.Add(FVector2D(
                (float)x / Resolution,
                (float)y / Resolution
            ));
        }
    }

    // COMPUTE TRIANGLES 
    Triangles.Empty(Resolution * Resolution * 6);
    FCosmicClipmapGeometry::GenerateClipmapLevelTriangles(Resolution, bIsRing, 0, Triangles);

    TArray<FVector> RotatedVertices;
    RotatedVertices.Reserve(TotalVertices);

    FMatrix TransformMatrix = PatchTransform.ToMatrixWithScale();

    for (size_t i = 0; i < TotalVertices; i++)
    {
        RotatedVertices.Add(TransformMatrix.TransformPosition(BaseVertices[i]));
    }

    CreateMeshSection_LinearColor(
        0,                    // SectionIndex
        RotatedVertices,      // Vertices
        Triangles,           // Triangles
        BaseNormals,      // Normals
        UVs,                 // UVs
        TArray<FLinearColor>(),    // Vertex colors
        BaseTangents,     // Tangents
        false
    );

    // VERIFY that it was created correctly
    if (GetNumSections() > 0)
    {
        bMeshCreated = true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("FALLÓ la creación de la malla!"));
    }


    SetCollisionEnabled(ECollisionEnabled::NoCollision);

}

void UCosmicMeshComponent::BuildSphereMesh()
{
    ClearAllMeshSections();

    BaseVertices.Empty();
    
    TArray<FVector> BaseNormals;
    TArray<FVector2D> UVs;
    TArray<FProcMeshTangent> BaseTangents;
    TArray<int32> Triangles;

    bIsSphereMesh = true;

    // Ensure multiple of 2
    Resolution = FMath::Max(4, Resolution & ~1);

    const int32 LatSegments = Resolution;
    const int32 LonSegments = Resolution * 2;

    const int32 VertResX = LonSegments + 1;
    const int32 VertResY = LatSegments + 1;

    const int32 TotalVertices = VertResX * VertResY;

    BaseVertices.Reserve(TotalVertices);
    BaseNormals.Reserve(TotalVertices);
    BaseTangents.Reserve(TotalVertices);
    UVs.Reserve(TotalVertices);

    // VERTICES
    for (int32 y = 0; y < VertResY; ++y)
    {
        float V = (float)y / LatSegments;
        float Theta = V * PI; // 0..PI

        float SinTheta = FMath::Sin(Theta);
        float CosTheta = FMath::Cos(Theta);

        for (int32 x = 0; x < VertResX; ++x)
        {
            float U = (float)x / LonSegments;
            float Phi = U * PI * 2.f; // 0..2PI

            float SinPhi = FMath::Sin(Phi);
            float CosPhi = FMath::Cos(Phi);

            FVector Normal(
                SinTheta * CosPhi,
                SinTheta * SinPhi,
                CosTheta
            );

            FVector Position = Normal * PlanetRadius;

            BaseVertices.Add(Position);
            BaseNormals.Add(Normal);

            FVector Tangent = FVector(-SinPhi, CosPhi, 0.f);
            Tangent.Normalize();
            BaseTangents.Add(FProcMeshTangent(Tangent, false));

            UVs.Add(FVector2D(U, V));
        }
    }

    // TRIANGLES
    for (int32 y = 0; y < LatSegments; ++y)
    {
        for (int32 x = 0; x < LonSegments; ++x)
        {
            int32 i0 = y * VertResX + x;
            int32 i1 = i0 + 1;
            int32 i2 = i0 + VertResX;
            int32 i3 = i2 + 1;

            // Counter-clockwise order from outside
            Triangles.Add(i0);
            Triangles.Add(i1);  
            Triangles.Add(i2);  

            Triangles.Add(i1);
            Triangles.Add(i3);  
            Triangles.Add(i2);  
        }
    }

    CreateMeshSection_LinearColor(
        0,                    // SectionIndex
        BaseVertices,      // Vertices
        Triangles,           // Triangles
        BaseNormals,      // Normals
        UVs,                 // UVs
        TArray<FLinearColor>(),    // Vertex colors
        BaseTangents,     // Tangents
        false      // Create collision
    );

    // VERIFY that it was created correctly
    if (GetNumSections() > 0)
    {
        bMeshCreated = true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("FALLÓ la creación de la malla!"));
    }


    SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void UCosmicMeshComponent::ReScaleLevel(int64 NewGridSpacing)
{
    if (!bMeshCreated)
    {
        UE_LOG(LogTemp, Error, TEXT("ReScaleLevel() llamado pero bMeshCreated = false"));
        return;
    }

    GridSpacing = NewGridSpacing;
    CachedHeights.Reset();
    CachedColors.Reset();
    CachedProjectionRevision = MAX_uint64;
}

void UCosmicMeshComponent::SetPositionAndRotation(const FVector& SurfacePos, const FRotator& PatchRotation)
{
    PatchTransform = FTransform(
        PatchRotation,
        SurfacePos, 
        FVector(1, 1, 1)
    );
}

void UCosmicMeshComponent::ConfigurePlanetaryProjection(
    const FTransform& InProjectionFrame,
    const FIntPoint& InGridCenter,
    uint64 InProjectionRevision,
    bool bInHasCoarserLevel)
{
    bUseSnappedPlanetProjection = true;
    bHasCoarserLevel = bInHasCoarserLevel;
    ProjectionFrame = InProjectionFrame;
    RequestedGridCenter = InGridCenter;
    RequestedProjectionRevision = InProjectionRevision;
}

bool UCosmicMeshComponent::IsPlanetaryProjectionUpdateRequired() const
{
    return !bUseSnappedPlanetProjection ||
        CachedProjectionRevision != RequestedProjectionRevision ||
        CachedGridCenter != RequestedGridCenter;
}

void UCosmicMeshComponent::SetMeshActive(bool active)
{
    bActiveMesh = active;
    SetMeshSectionVisible(0, active);
}

void UCosmicMeshComponent::RequestMeshUpdate(TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy)
{
    if (!bMeshCreated || bIsGeneratingNoise) return;

    bIsGeneratingNoise = true;

    // Planet center 
    FVector PlanetCenter = GetOwner()->GetActorLocation();

    FCosmicPlanetClipmapGenerationSettings ClipmapGenerationSettings;
    if (bUseSnappedPlanetProjection && bIsPlanet && !bIsSphereMesh)
    {
        ClipmapGenerationSettings.bEnabled = true;
        ClipmapGenerationSettings.bHasCoarserLevel = bHasCoarserLevel;
        ClipmapGenerationSettings.GridResolution = Resolution;
        ClipmapGenerationSettings.ProjectionFrame = ProjectionFrame;
        ClipmapGenerationSettings.DesiredGridCenter = RequestedGridCenter;
        ClipmapGenerationSettings.PreviousGridCenter = CachedGridCenter;
        ClipmapGenerationSettings.ProjectionRevision = RequestedProjectionRevision;
        ClipmapGenerationSettings.PreviousProjectionRevision = CachedProjectionRevision;
        ClipmapGenerationSettings.CachedHeights = MoveTemp(CachedHeights);
        ClipmapGenerationSettings.CachedColors = MoveTemp(CachedColors);
    }

    NoiseTask = new FAsyncTask<FCosmicNoiseGenerationTask>(
        BaseVertices,
        PatchTransform,
        PlanetCenter,
        PlanetRadius,
        GridSpacing,
        bIsPlanet,
        bIsSphereMesh,
        NoiseGenerationStrategy,
        MoveTemp(ClipmapGenerationSettings)
    );
    // Launch asynchronous task
    NoiseTask->StartBackgroundTask();
}



bool UCosmicMeshComponent::CheckAndApplyMeshUpdate()
{
    // If there is no task we return true to indicate it is free
    
    if (!NoiseTask) return true;
    // If not finished, return false
    if (!NoiseTask->IsDone()) return false;

    FCosmicNoiseGenerationTask& CompletedTask = NoiseTask->GetTask();
    TArray<FVector> CurrentVertices = MoveTemp(CompletedTask.CalculatedVertices);
    TArray<FLinearColor> CurrentColors = MoveTemp(CompletedTask.CalculatedColors);
    TArray<FVector> CurrentNormals;

    if (bIsPlanet || bIsSphereMesh) {
        CurrentNormals = MoveTemp(CompletedTask.CalculatedNormals);
    }

    if (bUseSnappedPlanetProjection && bIsPlanet && !bIsSphereMesh)
    {
        CachedHeights = MoveTemp(CompletedTask.CalculatedHeightCache);
        CachedColors = MoveTemp(CompletedTask.CalculatedColorCache);
        CachedGridCenter = CompletedTask.CalculatedGridCenter;
        CachedProjectionRevision = CompletedTask.CalculatedProjectionRevision;
    }
    
    // Clear task memory
    delete NoiseTask;
    NoiseTask = nullptr;

    bIsGeneratingNoise = false;

    // Fast update path using MoveTemp directly without FProcMeshVertex intermediate allocations
    UpdateMeshSection_FastVectors(
        MoveTemp(CurrentVertices),
        MoveTemp(CurrentNormals),
        MoveTemp(CurrentColors),
        false
    );

    return true; // Mesh has been updated
}

bool UCosmicMeshComponent::IsTaskActive()
{
    return NoiseTask && !NoiseTask->IsDone();
}

void UCosmicMeshComponent::CancelAsyncWork()
{

    bIsGeneratingNoise = false;

    // Cache is moved to task while it runs. If canceled it
    // cannot guarantee consistency and next request must regenerate it.
    CachedHeights.Reset();
    CachedColors.Reset();
    CachedProjectionRevision = MAX_uint64;

    if (NoiseTask == nullptr) return;

    if (NoiseTask->Cancel() || NoiseTask->IsDone())
    {
        delete NoiseTask;
    }
    else
    {
        NoiseTask->EnsureCompletion();
        delete NoiseTask;        
    }

    NoiseTask = nullptr;
}
