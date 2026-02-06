// Fill out your copyright notice in the Description page of Project Settings.


#include "Terrain/ClipmapMeshComponent.h"

void UClipmapMeshComponent::BuildMesh()
{
    ClearAllMeshSections();

    Vertices.Reset();
    Triangles.Reset();
    Normals.Reset();
    UVs.Reset();
    Tangents.Reset();

    const int32 VertRes = Resolution + 1;
    const int32 HalfRes = Resolution / 2;

    Vertices.Reserve(VertRes * VertRes);
    UVs.Reserve(VertRes * VertRes);

    // ------------------
    // VERTICES
    // ------------------
    for (int32 y = 0; y < VertRes; ++y)
    {
        for (int32 x = 0; x < VertRes; ++x)
        {
            float WorldX = (x - HalfRes) * GridSpacing;
            float WorldY = (y - HalfRes) * GridSpacing;

            float Height = 0.f; // altura plana por ahora

            Vertices.Add(FVector(WorldX, WorldY, Height));
            UVs.Add(FVector2D(
                (float)x / Resolution,
                (float)y / Resolution
            ));
            Normals.Add(FVector::UpVector);
            Tangents.Add(FProcMeshTangent(1, 0, 0));
        }
    }

    // ------------------
    // TRIÁNGULOS
    // ------------------
    for (int32 y = 0; y < Resolution; ++y)
    {
        for (int32 x = 0; x < Resolution; ++x)
        {
            // --- lógica del anillo ---
            if (bIsRing)
            {
                bool bInsideInner =
                    x > HalfRes / 2 &&
                    x < Resolution - HalfRes / 2 &&
                    y > HalfRes / 2 &&
                    y < Resolution - HalfRes / 2;

                if (bInsideInner)
                    continue;
            }

            int32 i0 = y * VertRes + x;
            int32 i1 = i0 + 1;
            int32 i2 = i0 + VertRes;
            int32 i3 = i2 + 1;

            Triangles.Add(i0);
            Triangles.Add(i2);
            Triangles.Add(i1);

            Triangles.Add(i1);
            Triangles.Add(i2);
            Triangles.Add(i3);
        }
    }

    // ------------------
    // CREAR MALLA
    // ------------------
    CreateMeshSection(
        0,
        Vertices,
        Triangles,
        Normals,
        UVs,
        TArray<FColor>(),
        Tangents,
        true
    );

    SetCollisionEnabled(ECollisionEnabled::NoCollision);
}


void UClipmapMeshComponent::UpdateHeights(const FVector2D& Origin)
{
}
