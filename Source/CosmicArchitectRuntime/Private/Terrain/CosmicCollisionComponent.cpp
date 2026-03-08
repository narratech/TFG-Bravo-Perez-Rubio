// Fill out your copyright notice in the Description page of Project Settings.


#include "Terrain/CosmicCollisionComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/BodyInstance.h"
#include "DrawDebugHelpers.h"

UCosmicCollisionComponent::UCosmicCollisionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    // Crear BodySetup
    BodySetup = NewObject<UBodySetup>(GetTransientPackage(), TEXT("CollisionBodySetup"), RF_Transient);
    BodySetup->CollisionTraceFlag = CTF_UseDefault;

    // Invisible
    SetVisibility(false);
    bHiddenInGame = true;
    SetCastShadow(false);
    SetGenerateOverlapEvents(false);

    // Marcar para reconstrucción
    bNeedsRebuild = true;

    CanBeCharacterBase = ECB_Yes;
}

void UCosmicCollisionComponent::OnRegister()
{
    Super::OnRegister();
    UpdateCollisionSettings();
}

void UCosmicCollisionComponent::OnUnregister()
{
    if (BodySetup)
    {
        BodySetup->AggGeom.EmptyElements();
    }
    Super::OnUnregister();
}

UBodySetup* UCosmicCollisionComponent::GetBodySetup()
{
    return BodySetup;
}

void UCosmicCollisionComponent::UpdateCollisionSettings()
{
    // Configurar BodyInstance
    BodyInstance.SetCollisionEnabled(CollisionEnabled);
    BodyInstance.SetObjectType(ObjectType);

    // Establecer respuestas por defecto
    BodyInstance.SetResponseToChannel(ECC_WorldStatic, ECR_Block);
    BodyInstance.SetResponseToChannel(ECC_WorldDynamic, ECR_Block);
    BodyInstance.SetResponseToChannel(ECC_Pawn, ECR_Block);
    BodyInstance.SetResponseToChannel(ECC_Vehicle, ECR_Block);
    BodyInstance.SetResponseToChannel(ECC_Destructible, ECR_Block);

    // Aplicar respuestas personalizadas
    for (auto& Pair : CustomResponses)
    {
        BodyInstance.SetResponseToChannel(Pair.Key, Pair.Value);
    }

    BodyInstance.bUseCCD = false;
}

void UCosmicCollisionComponent::DrawDebugCollisionMesh(const TArray<FVector>& Verts, const TArray<int32>& Tris)
{
    UWorld* World = GetWorld();
    if (!World) return;

    const FColor OrbitColor = DebugColor;
    const float OrbitThickness = DebugLineWidth;

    for (int32 i = 0; i < Tris.Num(); i += 3)
    {
        const FVector& A = Verts[Tris[i]];
        const FVector& B = Verts[Tris[i + 1]];
        const FVector& C = Verts[Tris[i + 2]];

        DrawDebugLine(World, A, B, OrbitColor, false, -1.0f, 0, OrbitThickness);
        DrawDebugLine(World, B, C, OrbitColor, false, -1.0f, 0, OrbitThickness);
        DrawDebugLine(World, C, A, OrbitColor, false, -1.0f, 0, OrbitThickness);
    }
}

void UCosmicCollisionComponent::RebuildCollision()
{
    if (CurrentCollisionCenter != FVector::ZeroVector)
    {
        bNeedsRebuild = true;
    }
}

void UCosmicCollisionComponent::GenerateCollisionMesh(
    const FVector& Center,
    const FVector& SurfaceNormal,
    float Radius,
    int32 Resolution)
{
    if (!BodySetup) return;

    // Si no ha cambiado significativamente y no requiere rebuild, saltar
    if (!bNeedsRebuild &&
        FVector::Dist(Center, CurrentCollisionCenter) < 100.0f &&
        FMath::Abs(Radius - CurrentCollisionRadius) < 100.0f)
    {
        return;
    }

    CurrentCollisionCenter = Center;
    CurrentCollisionRadius = Radius;
    bNeedsRebuild = false;

    BodySetup->AggGeom.EmptyElements();

    // Generar datos de malla
    TArray<FVector> Verts;
    TArray<int32> Tris;

    GenerateCollisionMeshData(Center, SurfaceNormal, Radius, Resolution, Verts, Tris);

    if (bShowCollisionMesh) {
        DrawDebugCollisionMesh(Verts, Tris);
    }
    
    if (Verts.Num() > 0 && Tris.Num() > 0)
    {
        // Crear elementos convexos para colisión
        if (!bGenerateComplexCollision)
        {
            FKConvexElem ConvexElem;

            // Crear una caja que aproxime el área
            float HalfSize = Radius * 0.5f;
            FVector BoxExtent(HalfSize, HalfSize, HalfSize * 0.1f);

            // Convertir a espacio local
            FVector LocalCenter = GetComponentTransform().InverseTransformPosition(Center);

            ConvexElem.VertexData.Add(FVector(-BoxExtent.X, -BoxExtent.Y, -BoxExtent.Z) + LocalCenter);
            ConvexElem.VertexData.Add(FVector(BoxExtent.X, -BoxExtent.Y, -BoxExtent.Z) + LocalCenter);
            ConvexElem.VertexData.Add(FVector(BoxExtent.X, BoxExtent.Y, -BoxExtent.Z) + LocalCenter);
            ConvexElem.VertexData.Add(FVector(-BoxExtent.X, BoxExtent.Y, -BoxExtent.Z) + LocalCenter);
            ConvexElem.VertexData.Add(FVector(-BoxExtent.X, -BoxExtent.Y, BoxExtent.Z) + LocalCenter);
            ConvexElem.VertexData.Add(FVector(BoxExtent.X, -BoxExtent.Y, BoxExtent.Z) + LocalCenter);
            ConvexElem.VertexData.Add(FVector(BoxExtent.X, BoxExtent.Y, BoxExtent.Z) + LocalCenter);
            ConvexElem.VertexData.Add(FVector(-BoxExtent.X, BoxExtent.Y, BoxExtent.Z) + LocalCenter);

            ConvexElem.UpdateElemBox();
            BodySetup->AggGeom.ConvexElems.Add(ConvexElem);
        }
        else
        {
            // Colisión compleja - varios convexos pequeños
            const int32 NumConvexHulls = 4;
            int32 VertsPerHull = FMath::Max(1, Verts.Num() / NumConvexHulls);

            for (int32 HullIdx = 0; HullIdx < NumConvexHulls; HullIdx++)
            {
                FKConvexElem ConvexElem;

                int32 StartIdx = HullIdx * VertsPerHull;
                int32 EndIdx = (HullIdx == NumConvexHulls - 1) ? Verts.Num() : StartIdx + VertsPerHull;

                for (int32 i = StartIdx; i < EndIdx; i++)
                {
                    // Transformar a espacio local del componente
                    FVector LocalVert = GetComponentTransform().InverseTransformPosition(Verts[i]);
                    ConvexElem.VertexData.Add(LocalVert);
                }

                if (ConvexElem.VertexData.Num() > 3)
                {
                    ConvexElem.UpdateElemBox();
                    BodySetup->AggGeom.ConvexElems.Add(ConvexElem);
                }
            }
        }
    }

    // Actualizar posición
    SetWorldLocation(Center);

    BodySetup->InvalidatePhysicsData();
    BodySetup->CreatePhysicsMeshes();

    // Recrear cuerpo físico
    BodyInstance.TermBody();
    RecreatePhysicsState();

    // Configurar colisiones
    UpdateCollisionSettings();
}

void UCosmicCollisionComponent::GenerateCollisionMeshData(
    const FVector& Center,
    const FVector& SurfaceNormal,
    float Radius,
    int32 Resolution,
    TArray<FVector>& OutVerts,
    TArray<int32>& OutTris)
{
    OutVerts.Empty();
    OutTris.Empty();

    // Crear sistema de coordenadas locales en la superficie
    FVector Up = SurfaceNormal;
    FVector Right = FVector::CrossProduct(FVector(0, 0, 1), Up);
    if (Right.SizeSquared() < 0.1f)
    {
        Right = FVector::CrossProduct(FVector(1, 0, 0), Up);
    }
    Right.Normalize();
    FVector Forward = FVector::CrossProduct(Up, Right);

    int32 VertRes = Resolution + 1;
    float GridSpacing = Radius / Resolution;

    // Ajustar espaciado según tamaño mínimo de triángulo
    if (GridSpacing < CollisionMinTriangleSize)
    {
        GridSpacing = CollisionMinTriangleSize;
        Resolution = FMath::FloorToInt(Radius / GridSpacing);
        VertRes = Resolution + 1;
    }

    float HalfSize = (Resolution * GridSpacing) * 0.5f;

    // Generar vértices
    for (int32 y = 0; y < VertRes; y++)
    {
        for (int32 x = 0; x < VertRes; x++)
        {
            float WorldX = (x - Resolution * 0.5f) * GridSpacing;
            float WorldY = (y - Resolution * 0.5f) * GridSpacing;

            // Posición en el plano local
            FVector LocalPos = Forward * WorldX + Right * WorldY;

            // Posición mundial
            FVector WorldPos = Center + LocalPos;

            OutVerts.Add(WorldPos);
        }
    }

    // Generar triángulos
    for (int32 y = 0; y < Resolution; y++)
    {
        for (int32 x = 0; x < Resolution; x++)
        {
            int32 i0 = y * VertRes + x;
            int32 i1 = i0 + 1;
            int32 i2 = i0 + VertRes;
            int32 i3 = i2 + 1;

            OutTris.Add(i0);
            OutTris.Add(i2);
            OutTris.Add(i1);

            OutTris.Add(i1);
            OutTris.Add(i2);
            OutTris.Add(i3);
        }
    }
}

void UCosmicCollisionComponent::ClearCollisionMesh()
{
    if (BodySetup)
    {
        BodySetup->AggGeom.EmptyElements();
        BodySetup->InvalidatePhysicsData();
    }
    BodyInstance.TermBody();
    CurrentCollisionCenter = FVector::ZeroVector;
}