// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "Terrain/CosmicCollisionGenerationTask.h"
#include "ICosmicNoiseStrategy.h"

FCosmicCollisionGenerationTask::FCosmicCollisionGenerationTask(
    const TArray<FVector>& InBaseVerts,
    const TArray<FVector>& InBaseNormals,
    const FTransform& InPatchTransform,
    const FVector& InPlanetCenter,
    TSharedPtr<ICosmicNoiseStrategy> InNoiseStrategy)
    : BaseVertices(InBaseVerts)
    , BaseNormals(InBaseNormals)
    , PatchTransform(InPatchTransform)
    , PlanetCenter(InPlanetCenter)
    , NoiseGenerationStrategy(InNoiseStrategy)
{
    CalculatedVertices.SetNumUninitialized(BaseVertices.Num());
}

void FCosmicCollisionGenerationTask::DoWork()
{
    const int32 NumVerts = BaseVertices.Num();
    if (NumVerts == 0 || !NoiseGenerationStrategy.IsValid())
    {
        return;
    }

    for (int32 i = 0; i < NumVerts; ++i)
    {
        const FVector WorldPos = PatchTransform.TransformPosition(BaseVertices[i]);
        const FVector NoiseDir = (WorldPos - PlanetCenter).GetSafeNormal();

        float FinalHeight = 0.0f;
        FLinearColor UnusedColor;
        NoiseGenerationStrategy->EvaluatePoint(NoiseDir, FinalHeight, UnusedColor);

        CalculatedVertices[i] = BaseVertices[i] + BaseNormals[i] * FinalHeight;
    }
}
