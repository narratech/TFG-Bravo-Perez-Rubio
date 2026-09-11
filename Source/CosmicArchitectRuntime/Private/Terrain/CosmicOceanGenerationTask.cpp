// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "Terrain/CosmicOceanGenerationTask.h"
#include "Terrain/CosmicClipmapGeometry.h"

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
                const double WorldX = (LevelCenter.X + (x - HalfRes)) * LevelSpacing;
                const double WorldY = (LevelCenter.Y + (y - HalfRes)) * LevelSpacing;

                FVector BasePosition;
                FVector Normal;
                FCosmicClipmapGeometry::ProjectPlanarGridPointToSphere(
                    WorldX, WorldY, OceanRadius, BasePosition, Normal);

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
