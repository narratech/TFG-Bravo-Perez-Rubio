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
    CalculatedBaseGridSpacing = Settings.BaseGridSpacing;
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
    const FMatrix TransformMatrix = Settings.PatchTransform.ToMatrixWithScale();

    // 1. EVALUATE RESCALING ON BACKGROUND THREAD
    const int32 SafeMinTriangleSize = FMath::Max(1, Settings.MinTriangleSize);
    int64 BaseGridSpacing = FMath::Max(static_cast<int64>(SafeMinTriangleSize), Settings.BaseGridSpacing);
    const int64 MaxSpacing = FMath::Max(BaseGridSpacing, Settings.MaxBaseGridSpacing);
    const double DistanceToSurface = Settings.DistanceToSurface;

    if (NumLevels > 1 && DistanceToSurface >= 0.0)
    {
        const int64 LastLevelSpacing = BaseGridSpacing * (1LL << (NumLevels - 1));
        const bool bLastVisible = FCosmicClipmapGeometry::IsClipmapRingVisible(
            LastLevelSpacing, Resolution, OceanRadius, DistanceToSurface);

        if (!bLastVisible && BaseGridSpacing > SafeMinTriangleSize)
        {
            const int32 Steps = FCosmicClipmapGeometry::CalculateDecreaseSteps(
                BaseGridSpacing, NumLevels, Resolution, OceanRadius, SafeMinTriangleSize, DistanceToSurface);
            const int64 Divisor = static_cast<int64>(1) << Steps;
            BaseGridSpacing = FMath::Max(static_cast<int64>(SafeMinTriangleSize), BaseGridSpacing / Divisor);
        }
        else if (FCosmicClipmapGeometry::IsClipmapRingVisible(
                     LastLevelSpacing * 2, Resolution, OceanRadius, DistanceToSurface)
                 && LastLevelSpacing < MaxSpacing * (1LL << (NumLevels - 1)))
        {
            const int32 Steps = FCosmicClipmapGeometry::CalculateIncreaseSteps(
                BaseGridSpacing, NumLevels, Resolution, OceanRadius, MaxSpacing, DistanceToSurface);
            const int64 Multiplier = static_cast<int64>(1) << Steps;
            BaseGridSpacing = FMath::Min(MaxSpacing, BaseGridSpacing * Multiplier);
        }
    }

    CalculatedBaseGridSpacing = BaseGridSpacing;

    // 2. RECOMPUTE COARSEST CENTER IF VIEWER COORDINATES ARE PROVIDED
    FIntPoint CoarsestCenter = Settings.CoarsestGridCenter;
    if (Settings.bUseViewerCoordinates)
    {
        const int64 CoarsestSpacing = BaseGridSpacing * (1LL << (NumLevels - 1));
        if (CoarsestSpacing > 0)
        {
            CoarsestCenter = FIntPoint(
                FMath::RoundToInt(Settings.ViewerCoordinates.X / static_cast<double>(CoarsestSpacing)),
                FMath::RoundToInt(Settings.ViewerCoordinates.Y / static_cast<double>(CoarsestSpacing))
            );
        }
    }

    CalculatedGridCenter = CoarsestCenter;

    // 3. GENERATE VERTICES AND NORMALS WITH RESCALED SPACING AND UPDATED CENTER
    for (int32 LevelIndex = 0; LevelIndex < NumLevels; ++LevelIndex)
    {
        const int64 LevelSpacing = BaseGridSpacing * (1LL << LevelIndex);
        const int64 LevelMultiplier = 1LL << ((NumLevels - 1) - LevelIndex);
        const FIntPoint LevelCenter(
            static_cast<int32>(static_cast<int64>(CoarsestCenter.X) * LevelMultiplier),
            static_cast<int32>(static_cast<int64>(CoarsestCenter.Y) * LevelMultiplier)
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

    CalculatedProjectionRevision = Settings.ProjectionRevision;
}
