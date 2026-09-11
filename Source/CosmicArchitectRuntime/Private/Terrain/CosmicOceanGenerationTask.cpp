// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "Terrain/CosmicOceanGenerationTask.h"

FCosmicOceanGenerationTask::FCosmicOceanGenerationTask(FCosmicOceanClipmapSettings InSettings)
    : Settings(MoveTemp(InSettings))
{
    const int32 NumLevels = FMath::Max(1, Settings.NumLevels);
    const int32 Resolution = FMath::Max(4, Settings.Resolution);
    const int32 VertRes = Resolution + 1;
    const int32 TotalVertices = NumLevels * VertRes * VertRes;

    CalculatedVertices.SetNumUninitialized(TotalVertices);
    CalculatedNormals.SetNumUninitialized(TotalVertices);
}

void FCosmicOceanGenerationTask::DoWork()
{
    const int32 NumLevels = FMath::Max(1, Settings.NumLevels);
    const int32 Resolution = FMath::Max(4, Settings.Resolution);
    const int32 VertRes = Resolution + 1;
    const int32 VerticesPerLevel = VertRes * VertRes;
    const int32 TotalVertices = NumLevels * VerticesPerLevel;
    const int32 HalfRes = Resolution / 2;

    if (CalculatedVertices.Num() != TotalVertices)
    {
        CalculatedVertices.SetNumUninitialized(TotalVertices);
        CalculatedNormals.SetNumUninitialized(TotalVertices);
    }

    const double OceanRadius = FMath::Max(100.0, Settings.OceanRadius);
    const int64 BaseGridSpacing = FMath::Max(static_cast<int64>(1), Settings.BaseGridSpacing);
    const FVector SphereCenter(0.0, 0.0, -OceanRadius);
    const FMatrix TransformMatrix = Settings.PatchTransform.ToMatrixWithScale();

    for (int32 LevelIndex = 0; LevelIndex < NumLevels; ++LevelIndex)
    {
        const int64 LevelSpacing = BaseGridSpacing * (1LL << LevelIndex);
        const int64 LevelMultiplier = 1LL << ((NumLevels - 1) - LevelIndex);
        const FIntPoint LevelCenter(
            static_cast<int32>(static_cast<int64>(Settings.CoarsestGridCenter.X) * LevelMultiplier),
            static_cast<int32>(static_cast<int64>(Settings.CoarsestGridCenter.Y) * LevelMultiplier)
        );

        const int32 LevelOffset = LevelIndex * VerticesPerLevel;

        for (int32 y = 0; y < VertRes; ++y)
        {
            const int32 RowOffset = LevelOffset + y * VertRes;

            for (int32 x = 0; x < VertRes; ++x)
            {
                // Exact formula from CosmicMeshComponent::BuildBaseProjectedMesh
                // with integer LevelCenter snapping offset
                const double WorldX = (LevelCenter.X + (x - HalfRes)) * LevelSpacing;
                const double WorldY = (LevelCenter.Y + (y - HalfRes)) * LevelSpacing;
                const double Distance2D = FMath::Sqrt(WorldX * WorldX + WorldY * WorldY);
                FVector BasePosition;

                if (Distance2D <= OceanRadius && Distance2D > 0.001)
                {
                    const double ZOffset = FMath::Sqrt(OceanRadius * OceanRadius - Distance2D * Distance2D);
                    BasePosition = FVector(WorldX, WorldY, -OceanRadius + ZOffset);
                }
                else if (Distance2D <= 0.001)
                {
                    BasePosition = FVector::ZeroVector;
                }
                else
                {
                    const double Scale = OceanRadius / Distance2D;
                    BasePosition = FVector(WorldX * Scale, WorldY * Scale, -OceanRadius);
                }

                // Normal from local sphere center
                FVector Normal = (BasePosition - SphereCenter);
                if (Normal.SizeSquared() > 0.001)
                {
                    Normal.Normalize();
                }
                else
                {
                    Normal = FVector::UpVector;
                }

                // Transform to planet space using PatchTransform
                const int32 VertexIndex = RowOffset + x;
                CalculatedVertices[VertexIndex] = TransformMatrix.TransformPosition(BasePosition);
                CalculatedNormals[VertexIndex] = TransformMatrix.TransformVector(Normal).GetSafeNormal();
            }
        }
    }

    CalculatedGridCenter = Settings.CoarsestGridCenter;
    CalculatedProjectionRevision = Settings.ProjectionRevision;
}
