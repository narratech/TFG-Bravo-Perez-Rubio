// Copyright 1998 - 2026 Epic Games, Inc. All Rights Reserved.


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
    TSharedPtr<ICosmicNoiseStrategy> InNoiseGenerationStrategy)
    : BaseVertices(InBaseVerts)
    , ComponentTransform(InTransform)
    , PlanetCenter(InPlanetCenter)
    , PlanetRadius(InPlanetRadius)
    , GridSpacing(InGridSpacing)
    , IsPlanet(InPlanet)
    , IsSphere(InIsSphere)
    , NoiseGenerationStrategy(InNoiseGenerationStrategy)
{
    // Reservamos memoria sin inicializar los elementos para optimizar rendimiento (SetNumUninitialized)
    CalculatedVertices.SetNumUninitialized(BaseVertices.Num());
    CalculatedColors.SetNumUninitialized(BaseVertices.Num()); 
    CalculatedNormals.SetNumUninitialized(BaseVertices.Num());
}

void FCosmicNoiseGenerationTask::DoWork()
{
    const int32 VertexCount = BaseVertices.Num();

    if (VertexCount <= 0) return;

    if (IsPlanet) {

        FMatrix TransformMatrix = ComponentTransform.ToMatrixWithScale();

        for (size_t i = 0; i < VertexCount; i++)
        {
            CalculatedVertices[i] = TransformMatrix.TransformPosition(BaseVertices[i]);
        }
    }

    // Loop de vértices
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

        FVector Normal;

        if (IsPlanet || IsSphere) {
            // Crear dos vectores perpendiculares a la dirección
            FVector Tangent1, Tangent2;
            NoiseDir.FindBestAxisVectors(Tangent1, Tangent2);

            // Generar puntos de muestra alrededor
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

        // Calcular posición final del vértice
        if (IsPlanet || IsSphere) {
            CalculatedNormals[i] = Normal;
            CalculatedVertices[i] += (NoiseDir * FinalHeight);
        }
        else {
            CalculatedVertices[i] = BaseVertices[i] + (FVector::UpVector * FinalHeight);
        }


        // Guardar colores
        CalculatedColors[i] = FinalColor;

    }
}
