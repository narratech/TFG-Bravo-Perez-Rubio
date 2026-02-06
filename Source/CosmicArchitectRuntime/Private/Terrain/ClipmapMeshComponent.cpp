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

    UE_LOG(LogTemp, Warning, TEXT("Radio Planeta: %f"), PlanetRadius);

    //Variable PlanetRadius

    // ------------------
    // VERTICES
    // ------------------
    // ------------------
// VERTICES
// ------------------
    // ------------------
// VERTICES
// ------------------
    for (int32 y = 0; y < VertRes; ++y)
    {
        for (int32 x = 0; x < VertRes; ++x)
        {
            float WorldX = (x - HalfRes) * GridSpacing;
            float WorldY = (y - HalfRes) * GridSpacing;

            // Calcular la posición proyectada en la esfera
            FVector SphereCenter = FVector(0, 0, -PlanetRadius);

            // Vector desde el centro de la esfera al punto en el plano XY
            FVector ToPoint = FVector(WorldX, WorldY, 0) - SphereCenter;

            // Solo queremos la cara superior (z > -Radius)
            FVector SpherePosition;

            // Calcular distancia 2D desde el eje de la esfera
            float Distance2D = FMath::Sqrt(WorldX * WorldX + WorldY * WorldY);

            if (Distance2D <= PlanetRadius)
            {
                // Punto dentro del radio: está sobre la superficie esférica
                float ZOffset = FMath::Sqrt(PlanetRadius * PlanetRadius - Distance2D * Distance2D);
                SpherePosition = FVector(WorldX, WorldY, -PlanetRadius + ZOffset);
            }
            else
            {
                // Punto fuera del radio: proyectar al borde del círculo en z = -Radius
                float Scale = PlanetRadius / Distance2D;
                SpherePosition = FVector(WorldX * Scale, WorldY * Scale, -PlanetRadius);
            }

            // IMPORTANTE: Usar las coordenadas proyectadas, no las originales
            Vertices.Add(SpherePosition);

            UVs.Add(FVector2D(
                (float)x / Resolution,
                (float)y / Resolution
            ));

            // Calcular la normal en la superficie de la esfera
            FVector Normal = (SpherePosition - SphereCenter);
            Normal.Normalize();

            Normals.Add(Normal);

            // Calcular tangente (aproximación)
            FVector Tangent = FVector(-Normal.Y, Normal.X, 0);
            Tangent.Normalize();
            Tangents.Add(FProcMeshTangent(Tangent.X, Tangent.Y, Tangent.Z));
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
