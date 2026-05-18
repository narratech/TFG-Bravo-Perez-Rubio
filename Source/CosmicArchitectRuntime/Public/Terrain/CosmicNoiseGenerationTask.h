// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"

class ICosmicNoiseStrategy;

/**
 * 
 */
class COSMICARCHITECTRUNTIME_API FCosmicNoiseGenerationTask : public FNonAbandonableTask
{
public:
    // Referencias a los datos inmutables de la malla
    const TArray<FVector>& BaseVertices;

    // El array donde guardaremos el resultado
    TArray<FVector> CalculatedVertices;
    TArray<FVector> CalculatedNormals;
    TArray<FLinearColor> CalculatedColors;

    // Datos de transformación
    FTransform ComponentTransform;
    FVector PlanetCenter;
    double PlanetRadius;
    double GridSpacing;

    bool IsPlanet;
    bool IsSphere;

    TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy;

    FCosmicNoiseGenerationTask(
        const TArray<FVector>& InBaseVerts,
        FTransform InTransform,
        FVector InPlanetCenter,
        double InPlanetRadius,
        double InGridSpacing,
        bool InPlanet,
        bool InIsSphere,
        TSharedPtr<ICosmicNoiseStrategy> InNoiseGenerationStrategy
    );

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(FCosmicArchitectNoiseGenerator, STATGROUP_ThreadPoolAsyncTasks);
    }

    void DoWork();
};
