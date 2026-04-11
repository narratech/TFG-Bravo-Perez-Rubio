
#include "Terrain/CosmicCollisionComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "PhysicsEngine/BodyInstance.h"
#include "CosmicNoiseSettings.h"
#include "DrawDebugHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "CosmicNoise.h"
#include "Engine/World.h"

UCosmicCollisionComponent::UCosmicCollisionComponent()
{
    bTickInEditor = true;

    PrimaryComponentTick.bCanEverTick = true;
}

void UCosmicCollisionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearCollision();

    Super::EndPlay(EndPlayReason);
}

void UCosmicCollisionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bShowCollisionMesh) {
        DrawDebugCollisionMesh();
    }
}

#if WITH_EDITOR
void UCosmicCollisionComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const FName PropertyName = PropertyChangedEvent.Property
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    // CAMBIOS QUE ROMPEN LA GEOMETRIA (REBUILD COMPLETO)
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicCollisionComponent, CollisionTriangleSize) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicCollisionComponent, CollisionResolution))
    {
        ClearCollision();

        if (AActor* Owner = GetOwner())
        {
            GenerateCollisionMesh(PlanetRadius);
        }

        return;
    }

    // CAMBIOS EN CONFIG DE FISICA (RECOOK)
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicCollisionComponent, bUseComplexAsSimpleCollision) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicCollisionComponent, bUseAsyncCooking))
    {
        if (IsBuilt())
        {
            BuildCollision();
        }
        return;
    }

}
#endif


void UCosmicCollisionComponent::RebuildCollision()
{
    bNeedsRebuild = true;
}

void UCosmicCollisionComponent::GenerateCollisionMesh(double Radius)
{
    if (bIsActive) return;

    const int32 VertRes = CollisionResolution + 1;
    const int32 TotalVertices = VertRes * VertRes;
    const int32 HalfRes = CollisionResolution / 2;

    PlanetRadius = Radius;

    BaseVertices.Empty();
    BaseNormals.Empty();

    BaseVertices.Reserve(TotalVertices);
    BaseNormals.Reserve(TotalVertices);

    // 3. CALCULAR VÉRTICES
    int32 ActualVerticesCalculated = 0;

    for (int32 y = 0; y < VertRes; ++y)
    {
        for (int32 x = 0; x < VertRes; ++x)
        {
            double WorldX = (x - HalfRes) * CollisionTriangleSize;
            double WorldY = (y - HalfRes) * CollisionTriangleSize;

            // Calcular posición en esfera
            FVector SphereCenter = FVector(0, 0, -Radius);
            double Distance2D = FMath::Sqrt(WorldX * WorldX + WorldY * WorldY);
            FVector BasePosition;

            if (Distance2D <= Radius && Distance2D > 0.001f) // Evitar división por 0
            {
                double ZOffset = FMath::Sqrt(Radius * Radius - Distance2D * Distance2D);
                BasePosition = FVector(WorldX, WorldY, -Radius + ZOffset);
            }
            else if (Distance2D <= 0.001f)
            {
                // Centro - evitar NaN
                BasePosition = FVector(0, 0, 0);
            }
            else
            {
                double Scale = Radius / Distance2D;
                BasePosition = FVector(WorldX * Scale, WorldY * Scale, -Radius);
            }

            BaseVertices.Add(BasePosition);
            ActualVerticesCalculated++;

            // Normal
            FVector Normal = (BasePosition - SphereCenter);
            if (Normal.SizeSquared() > 0.001f)
            {
                Normal.Normalize();
            }
            else
            {
                Normal = FVector::UpVector;
            }
            BaseNormals.Add(Normal);
        }
    }

    //UE_LOG(LogTemp, Warning, TEXT("Creando colision"));

    // 4. CALCULAR TRIÁNGULOS (CORREGIDO)
    Tris.Empty();
    int32 TriangleCount = 0;

    for (int32 y = 0; y < CollisionResolution; ++y)
    {
        for (int32 x = 0; x < CollisionResolution; ++x)
        {
            // Índices de vértices
            int32 i0 = y * VertRes + x;
            int32 i1 = i0 + 1;
            int32 i2 = i0 + VertRes;
            int32 i3 = i2 + 1;

            if (i0 >= TotalVertices || i1 >= TotalVertices ||
                i2 >= TotalVertices || i3 >= TotalVertices)
            {
                UE_LOG(LogTemp, Error, TEXT("Índice de triángulo inválido en [%d,%d]"), x, y);
                continue;
            }

            Tris.Add(i0);
            Tris.Add(i2);
            Tris.Add(i1);

            Tris.Add(i1);
            Tris.Add(i2);
            Tris.Add(i3);
        }
    }

    Verts = BaseVertices;

    BuildCollision();
}

void UCosmicCollisionComponent::UpdateCollisionMesh(UCosmicNoiseSettings* NoiseSettings)
{
    if (!bIsActive) return;
    //double CreateStartTime = FPlatformTime::Seconds();

    TArray<float> Heights = CosmicNoise::CalculateHeights(BaseVertices, GetOwner()->GetActorLocation(), GetComponentTransform(), NoiseSettings);

    for (size_t i = 0; i < Heights.Num(); i++)
    {
        Verts[i] = BaseVertices[i] + BaseNormals[i] * Heights[i];
    }

    UpdateCollisionVertices();

    //double CreateEndTime = FPlatformTime::Seconds();

    //UE_LOG(LogTemp, Warning, TEXT("Actualizar malla de colision tomo: %.4f ms"), (CreateEndTime - CreateStartTime) * 1000.0);
}

void UCosmicCollisionComponent::ClearCollision() 
{   
    // Cancelar async cooking
    for (UBodySetup* Setup : AsyncBodySetupQueue)
    {
        if (Setup)
        {
            Setup->AbortPhysicsMeshAsyncCreation();
        }
    }
    AsyncBodySetupQueue.Empty();

    // Limpiar BodySetup actual
    if (BodySetup)
    {
        BodySetup->ClearPhysicsMeshes();
        BodySetup = nullptr;
    }

    // Quitar del sistema de físicas
    DestroyPhysicsState();

    // Limpiar datos
    Verts.Empty();
    Tris.Empty();
    BaseVertices.Empty();
    BaseNormals.Empty();

    bIsActive = false;
    bNeedsRebuild = false;
}

bool UCosmicCollisionComponent::IsBuilt() const 
{
    return bIsActive;
}

void UCosmicCollisionComponent::DrawDebugCollisionMesh()
{
    UWorld* World = GetWorld();
    if (!World) return;

    const FColor OrbitColor = DebugColor;
    const float OrbitThickness = DebugLineWidth;

    for (int32 i = 0; i < Tris.Num(); i += 3) {

        FVector A = GetComponentTransform().TransformPosition(Verts[Tris[i]]);
        FVector B = GetComponentTransform().TransformPosition(Verts[Tris[i + 1]]);
        FVector C = GetComponentTransform().TransformPosition(Verts[Tris[i + 2]]);

        DrawDebugLine(World, A, B, OrbitColor, false, -1.0f, 0, OrbitThickness);
        DrawDebugLine(World, B, C, OrbitColor, false, -1.0f, 0, OrbitThickness);
        DrawDebugLine(World, C, A, OrbitColor, false, -1.0f, 0, OrbitThickness);
    }
}

UBodySetup* UCosmicCollisionComponent::CreateBodySetupHelper()
{
    UBodySetup* NewBodySetup = NewObject<UBodySetup>(this);

    NewBodySetup->BodySetupGuid = FGuid::NewGuid();
    NewBodySetup->bGenerateMirroredCollision = false;
    NewBodySetup->bDoubleSidedGeometry = true;

    NewBodySetup->CollisionTraceFlag =
        bUseComplexAsSimpleCollision ?
        CTF_UseComplexAsSimple :
        CTF_UseDefault;

    return NewBodySetup;
}

void UCosmicCollisionComponent::CreateProcMeshBodySetup()
{
    if (!BodySetup)
    {
        BodySetup = CreateBodySetupHelper();
    }
}

UBodySetup* UCosmicCollisionComponent::GetBodySetup()
{
    if (!BodySetup)
    {
        CreateProcMeshBodySetup();
    }

    return BodySetup;
}

void UCosmicCollisionComponent::BuildCollision()
{
    if (Verts.Num() == 0 || Tris.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Collision mesh empty"));
        return;
    }

    bIsActive = true;

    UWorld* World = GetWorld();

    bool bAsync = World && World->IsGameWorld() && bUseAsyncCooking;

    if (bAsync)
    {
        for (UBodySetup* OldBody : AsyncBodySetupQueue)
        {
            if (OldBody)
                OldBody->AbortPhysicsMeshAsyncCreation();
        }

        AsyncBodySetupQueue.Add(CreateBodySetupHelper());
    }
    else
    {
        AsyncBodySetupQueue.Empty();
        CreateProcMeshBodySetup();
    }

    UBodySetup* UseBodySetup =
        bAsync ? AsyncBodySetupQueue.Last() : BodySetup;

    UseBodySetup->CollisionTraceFlag =
        bUseComplexAsSimpleCollision ?
        CTF_UseComplexAsSimple :
        CTF_UseDefault;

    if (bAsync)
    {
        UseBodySetup->CreatePhysicsMeshesAsync(
            FOnAsyncPhysicsCookFinished::CreateUObject(
                this,
                &UCosmicCollisionComponent::FinishPhysicsAsyncCook,
                UseBodySetup));
    }
    else
    {
        UseBodySetup->InvalidatePhysicsData();
        UseBodySetup->CreatePhysicsMeshes();
        RecreatePhysicsState();
    }
}

void UCosmicCollisionComponent::UpdateCollisionVertices()
{
    if (Verts.Num() == 0)
        return;

    if (BodyInstance.IsValidBodyInstance())
    {
        BodyInstance.UpdateTriMeshVertices(Verts);
    }
}

void UCosmicCollisionComponent::FinishPhysicsAsyncCook(bool bSuccess, UBodySetup* FinishedBodySetup)
{
    if (bSuccess)
    {
        BodySetup = FinishedBodySetup;
        RecreatePhysicsState();
    }

    AsyncBodySetupQueue.Remove(FinishedBodySetup);
}

bool UCosmicCollisionComponent::GetPhysicsTriMeshData(
    FTriMeshCollisionData* CollisionData,
    bool InUseAllTriData)
{
    if (!CollisionData) return false;

    bool bCopyUVs = UPhysicsSettings::Get()->bSupportUVFromHitResults;

    if (bCopyUVs)
        CollisionData->UVs.AddZeroed(1);

    for (const FVector& V : Verts)
    {
        CollisionData->Vertices.Add((FVector3f)V);

        if (bCopyUVs)
            CollisionData->UVs[0].Add(FVector2D::ZeroVector);
    }

    int32 NumTris = Tris.Num() / 3;

    for (int32 i = 0; i < NumTris; i++)
    {
        FTriIndices Tri;
        Tri.v0 = Tris[i * 3 + 0];
        Tri.v1 = Tris[i * 3 + 1];
        Tri.v2 = Tris[i * 3 + 2];

        CollisionData->Indices.Add(Tri);
        CollisionData->MaterialIndices.Add(0);
    }

    CollisionData->bFlipNormals = true;
    CollisionData->bFastCook = true;

    return true;
}

bool UCosmicCollisionComponent::ContainsPhysicsTriMeshData(bool InUseAllTriData) const
{
    return Tris.Num() >= 3;
}

bool UCosmicCollisionComponent::GetTriMeshSizeEstimates(
    FTriMeshCollisionDataEstimates& OutTriMeshEstimates,
    bool bInUseAllTriData) const
{
    OutTriMeshEstimates.VerticeCount = Verts.Num();
    return true;
}