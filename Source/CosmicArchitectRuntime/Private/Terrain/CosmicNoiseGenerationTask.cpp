// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.


#include "Terrain/CosmicNoiseGenerationTask.h"
#include "ICosmicNoiseStrategy.h"

FCosmicNoiseGenerationTask::FCosmicNoiseGenerationTask(
    const TArray<FVector>& InBaseVerts,
    FTransform InTransform,
    FVector InPlanetCenter,
    double InPlanetRadius,
    double InGridSpacing,
    bool InPlanet,
    bool InIsSphere,
    TSharedPtr<ICosmicNoiseStrategy> InNoiseGenerationStrategy,
    FCosmicPlanetClipmapGenerationSettings InClipmapSettings)
    : BaseVertices(InBaseVerts)
    , ComponentTransform(InTransform)
    , PlanetCenter(InPlanetCenter)
    , PlanetRadius(InPlanetRadius)
    , GridSpacing(InGridSpacing)
    , IsPlanet(InPlanet)
    , IsSphere(InIsSphere)
    , NoiseGenerationStrategy(InNoiseGenerationStrategy)
    , ClipmapSettings(MoveTemp(InClipmapSettings))
{
    // Reserve memory without initializing elements for performance optimization (SetNumUninitialized)
    CalculatedVertices.SetNumUninitialized(BaseVertices.Num());
    CalculatedColors.SetNumUninitialized(BaseVertices.Num()); 
    CalculatedNormals.SetNumUninitialized(BaseVertices.Num());
}

void FCosmicNoiseGenerationTask::DoWork()
{
    if (ClipmapSettings.bEnabled && IsPlanet && !IsSphere)
    {
        DoSnappedPlanetClipmapWork();
        return;
    }

    const int32 VertexCount = BaseVertices.Num();

    if (VertexCount <= 0) return;

    if (IsPlanet) {

        FMatrix TransformMatrix = ComponentTransform.ToMatrixWithScale();

        for (size_t i = 0; i < VertexCount; i++)
        {
            CalculatedVertices[i] = TransformMatrix.TransformPosition(BaseVertices[i]);
        }
    }

    // Vertex loop
    for (int32 i = 0; i < VertexCount; i++)
    {
        FVector WorldPos = IsPlanet ? CalculatedVertices[i] : BaseVertices[i];
        FVector NoiseDir = IsPlanet || IsSphere ? WorldPos.GetSafeNormal() : FVector(WorldPos.X, WorldPos.Y, 0);

        if (IsSphere) {
            CalculatedVertices[i] = WorldPos;
        }

        float FinalHeight;
        FLinearColor FinalColor;

        const double SampleDistance = GridSpacing;

        FVector Normal = FVector::ZeroVector;

        if (IsPlanet || IsSphere) {
            // Create two vectors perpendicular to direction
            FVector Tangent1, Tangent2;
            NoiseDir.FindBestAxisVectors(Tangent1, Tangent2);

            // Generate sample points around
            FVector SampleDirs[] = {
                (NoiseDir + Tangent1 * (SampleDistance / PlanetRadius)).GetSafeNormal(),
                (NoiseDir - Tangent1 * (SampleDistance / PlanetRadius)).GetSafeNormal(),
                (NoiseDir + Tangent2 * (SampleDistance / PlanetRadius)).GetSafeNormal(),
                (NoiseDir - Tangent2 * (SampleDistance / PlanetRadius)).GetSafeNormal()
            };

            float Heights[4];
            FLinearColor Dummy;

            for (int j = 0; j < 4; j++)
            {
                NoiseGenerationStrategy->EvaluatePoint(SampleDirs[j], Heights[j], Dummy);
            }

            float dH_dT1 = (Heights[0] - Heights[1]) / (2.0f * SampleDistance);
            float dH_dT2 = (Heights[2] - Heights[3]) / (2.0f * SampleDistance);

            FVector dP_dT1 = Tangent1 + NoiseDir * dH_dT1;
            FVector dP_dT2 = Tangent2 + NoiseDir * dH_dT2;

            Normal = FVector::CrossProduct(dP_dT2, dP_dT1).GetSafeNormal();
        }

        NoiseGenerationStrategy->EvaluatePoint(NoiseDir, FinalHeight, FinalColor);

        // Compute final vertex position
        if (IsPlanet || IsSphere) {
            CalculatedNormals[i] = Normal;
            CalculatedVertices[i] += (NoiseDir * FinalHeight);
        }
        else {
            CalculatedVertices[i] = BaseVertices[i] + (FVector::UpVector * FinalHeight);
        }


        // Store colors
        CalculatedColors[i] = FinalColor;

    }
}

void FCosmicNoiseGenerationTask::DoSnappedPlanetClipmapWork()
{
    constexpr int32 CacheBorder = 2;

    const int32 Resolution = ClipmapSettings.GridResolution;
    const int32 VertexResolution = Resolution + 1;
    const int32 ExpectedVertexCount = VertexResolution * VertexResolution;
    const int32 CacheResolution = VertexResolution + CacheBorder * 2;
    const int32 CacheCount = CacheResolution * CacheResolution;

    if (Resolution <= 0 || BaseVertices.Num() != ExpectedVertexCount || !NoiseGenerationStrategy.IsValid())
    {
        CalculatedVertices.Reset();
        CalculatedNormals.Reset();
        CalculatedColors.Reset();
        return;
    }

    CalculatedVertices.SetNumUninitialized(ExpectedVertexCount);
    CalculatedNormals.SetNumUninitialized(ExpectedVertexCount);
    CalculatedColors.SetNumUninitialized(ExpectedVertexCount);

    TArray<float> PreviousHeights = MoveTemp(ClipmapSettings.CachedHeights);
    TArray<FLinearColor> PreviousColors = MoveTemp(ClipmapSettings.CachedColors);

    CalculatedHeightCache.SetNumUninitialized(CacheCount);
    CalculatedColorCache.SetNumUninitialized(CacheCount);

    const bool bCanReuseCache =
        ClipmapSettings.PreviousProjectionRevision == ClipmapSettings.ProjectionRevision &&
        PreviousHeights.Num() == CacheCount &&
        PreviousColors.Num() == CacheCount;

    const FIntPoint CellDelta =
        ClipmapSettings.DesiredGridCenter - ClipmapSettings.PreviousGridCenter;
    const int32 HalfResolution = Resolution / 2;

    auto CacheIndex = [CacheResolution](int32 X, int32 Y)
    {
        return Y * CacheResolution + X;
    };

    auto DirectionAtCacheCoordinate = [this, HalfResolution](int32 CacheX, int32 CacheY)
    {
        const int32 GlobalCellX = ClipmapSettings.DesiredGridCenter.X
            + CacheX - CacheBorder - HalfResolution;
        const int32 GlobalCellY = ClipmapSettings.DesiredGridCenter.Y
            + CacheY - CacheBorder - HalfResolution;

        const double PlaneX = static_cast<double>(GlobalCellX) * GridSpacing;
        const double PlaneY = static_cast<double>(GlobalCellY) * GridSpacing;
        const double NormalizedX = PlaneX / PlanetRadius;
        const double NormalizedY = PlaneY / PlanetRadius;
        const double RadiusSquared =
            NormalizedX * NormalizedX + NormalizedY * NormalizedY;

        // Inverse of an orthographic projection onto the sphere. Preserves
        // physical spacing near observer and reaches horizon when
        // plane reaches planet radius, same as previous mesh.
        FVector LocalDirection;
        if (RadiusSquared < 1.0)
        {
            LocalDirection = FVector(
                NormalizedX,
                NormalizedY,
                FMath::Sqrt(1.0 - RadiusSquared));
        }
        else
        {
            LocalDirection = FVector(NormalizedX, NormalizedY, 0.0).GetSafeNormal();
        }

        return ClipmapSettings.ProjectionFrame
            .TransformVectorNoScale(LocalDirection)
            .GetSafeNormal();
    };

    for (int32 CacheY = 0; CacheY < CacheResolution; ++CacheY)
    {
        for (int32 CacheX = 0; CacheX < CacheResolution; ++CacheX)
        {
            const int32 NewIndex = CacheIndex(CacheX, CacheY);
            const int32 OldX = CacheX + CellDelta.X;
            const int32 OldY = CacheY + CellDelta.Y;

            if (bCanReuseCache &&
                OldX >= 0 && OldX < CacheResolution &&
                OldY >= 0 && OldY < CacheResolution)
            {
                const int32 OldIndex = CacheIndex(OldX, OldY);
                CalculatedHeightCache[NewIndex] = PreviousHeights[OldIndex];
                CalculatedColorCache[NewIndex] = PreviousColors[OldIndex];
                continue;
            }

            const FVector NoiseDirection = DirectionAtCacheCoordinate(CacheX, CacheY);
            NoiseGenerationStrategy->EvaluatePoint(
                NoiseDirection,
                CalculatedHeightCache[NewIndex],
                CalculatedColorCache[NewIndex]);
        }
    }

    TArray<FVector> CachedPositions;
    CachedPositions.SetNumUninitialized(CacheCount);

    for (int32 CacheY = 0; CacheY < CacheResolution; ++CacheY)
    {
        for (int32 CacheX = 0; CacheX < CacheResolution; ++CacheX)
        {
            const int32 Index = CacheIndex(CacheX, CacheY);
            const FVector Direction = DirectionAtCacheCoordinate(CacheX, CacheY);
            CachedPositions[Index] = Direction * (PlanetRadius + CalculatedHeightCache[Index]);
        }
    }

    auto PositionAtGlobalCell =
        [this, &CachedPositions, CacheIndex, HalfResolution](int32 GlobalX, int32 GlobalY)
    {
        const int32 CacheX = GlobalX - ClipmapSettings.DesiredGridCenter.X
            + HalfResolution + CacheBorder;
        const int32 CacheY = GlobalY - ClipmapSettings.DesiredGridCenter.Y
            + HalfResolution + CacheBorder;
        return CachedPositions[CacheIndex(CacheX, CacheY)];
    };

    const int32 TransitionWidth = FMath::Max(2, Resolution / 10);
    const int32 TransitionStart = HalfResolution - TransitionWidth;

    for (int32 VertexY = 0; VertexY < VertexResolution; ++VertexY)
    {
        for (int32 VertexX = 0; VertexX < VertexResolution; ++VertexX)
        {
            const int32 VertexIndex = VertexY * VertexResolution + VertexX;
            const int32 CacheX = VertexX + CacheBorder;
            const int32 CacheY = VertexY + CacheBorder;
            const int32 CenterIndex = CacheIndex(CacheX, CacheY);

            const FVector& FinePosition = CachedPositions[CenterIndex];
            const FVector FineDx = CachedPositions[CacheIndex(CacheX + 1, CacheY)]
                - CachedPositions[CacheIndex(CacheX - 1, CacheY)];
            const FVector FineDy = CachedPositions[CacheIndex(CacheX, CacheY + 1)]
                - CachedPositions[CacheIndex(CacheX, CacheY - 1)];
            FVector FineNormal = FVector::CrossProduct(FineDx, FineDy).GetSafeNormal();
            if (FVector::DotProduct(FineNormal, FinePosition) < 0.0)
            {
                FineNormal *= -1.0;
            }

            float TransitionAlpha = 0.0f;
            if (ClipmapSettings.bHasCoarserLevel)
            {
                const int32 DistanceFromCenter = FMath::Max(
                    FMath::Abs(VertexX - HalfResolution),
                    FMath::Abs(VertexY - HalfResolution));
                TransitionAlpha = FMath::Clamp(
                    static_cast<float>(DistanceFromCenter - TransitionStart) /
                    static_cast<float>(TransitionWidth),
                    0.0f,
                    1.0f);
            }

            FVector FinalPosition = FinePosition;
            FVector FinalNormal = FineNormal;

            if (TransitionAlpha > 0.0f)
            {
                const int32 GlobalX = ClipmapSettings.DesiredGridCenter.X
                    + VertexX - HalfResolution;
                const int32 GlobalY = ClipmapSettings.DesiredGridCenter.Y
                    + VertexY - HalfResolution;
                const int32 CoarseX0 = FMath::FloorToInt(static_cast<double>(GlobalX) * 0.5) * 2;
                const int32 CoarseY0 = FMath::FloorToInt(static_cast<double>(GlobalY) * 0.5) * 2;
                const float FractionX = static_cast<float>(GlobalX - CoarseX0) * 0.5f;
                const float FractionY = static_cast<float>(GlobalY - CoarseY0) * 0.5f;

                const FVector CoarseX0Position = FMath::Lerp(
                    PositionAtGlobalCell(CoarseX0, CoarseY0),
                    PositionAtGlobalCell(CoarseX0 + 2, CoarseY0),
                    FractionX);
                const FVector CoarseX1Position = FMath::Lerp(
                    PositionAtGlobalCell(CoarseX0, CoarseY0 + 2),
                    PositionAtGlobalCell(CoarseX0 + 2, CoarseY0 + 2),
                    FractionX);
                const FVector CoarsePosition = FMath::Lerp(
                    CoarseX0Position,
                    CoarseX1Position,
                    FractionY);

                const FVector CoarseDx = CachedPositions[CacheIndex(CacheX + 2, CacheY)]
                    - CachedPositions[CacheIndex(CacheX - 2, CacheY)];
                const FVector CoarseDy = CachedPositions[CacheIndex(CacheX, CacheY + 2)]
                    - CachedPositions[CacheIndex(CacheX, CacheY - 2)];
                FVector CoarseNormal = FVector::CrossProduct(CoarseDx, CoarseDy).GetSafeNormal();
                if (FVector::DotProduct(CoarseNormal, FinePosition) < 0.0)
                {
                    CoarseNormal *= -1.0;
                }

                FinalPosition = FMath::Lerp(FinePosition, CoarsePosition, TransitionAlpha);
                FinalNormal = FMath::Lerp(FineNormal, CoarseNormal, TransitionAlpha).GetSafeNormal();
            }

            CalculatedVertices[VertexIndex] = FinalPosition;
            CalculatedNormals[VertexIndex] = FinalNormal;
            CalculatedColors[VertexIndex] = CalculatedColorCache[CenterIndex];
        }
    }

    CalculatedGridCenter = ClipmapSettings.DesiredGridCenter;
    CalculatedProjectionRevision = ClipmapSettings.ProjectionRevision;
}
