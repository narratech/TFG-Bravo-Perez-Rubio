// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicNoise.h"
#include "ThirdParty/FastNoiseLite.h"
#include "CosmicNoiseTypes.h"
#include "CosmicNoiseEvaluator.h"


TArray<float> CosmicNoise::CalculateHeights(const TArray<FVector>& Points, const FVector& PlanetCenter, const FTransform& ComponentTransform, FCosmicNoiseGenerationParameters Settings)
{
    TArray<float> OutHeights;
    if (Points.IsEmpty()) return OutHeights;

    OutHeights.SetNumUninitialized(Points.Num());

    FCosmicNoiseEvaluator Evaluator(Settings);

    for (int32 i = 0; i < Points.Num(); i++)
    {
        FVector WorldPos = ComponentTransform.TransformPosition(Points[i]);
        FVector NoiseDir = (WorldPos - PlanetCenter).GetSafeNormal();

        float FinalHeight;
        FLinearColor FinalColor; // Se calcula pero lo ignoramos para colisiones

        Evaluator.EvaluatePoint(NoiseDir, FinalHeight, FinalColor);

        OutHeights[i] = FinalHeight;
    }

    return OutHeights;
}

TArray<float> CosmicNoise::CalculateHeightsDirect(const TArray<FVector>& Points, FCosmicNoiseGenerationParameters Settings)
{
    TArray<float> OutHeights;

    // Comprobación de seguridad
    if (Points.IsEmpty())
    {
        return OutHeights;
    }

    OutHeights.SetNumUninitialized(Points.Num());

    FCosmicNoiseEvaluator Evaluator(Settings);

    // Loop de puntos
    for (int32 i = 0; i < Points.Num(); i++)
    {
        float X = Points[i].X;
        float Y = Points[i].Y;
        float Z = Points[i].Z;

        FVector NoiseDir = FVector(X, Y, Z);

        float FinalHeight;
        FLinearColor FinalColor; // Se calcula pero lo ignoramos para colisiones

        Evaluator.EvaluatePoint(NoiseDir, FinalHeight, FinalColor);

        OutHeights[i] = FinalHeight;
    }

    return OutHeights;
}

