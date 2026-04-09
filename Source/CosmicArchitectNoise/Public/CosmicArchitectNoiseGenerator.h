#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"
#include "ThirdParty/FastNoiseLite.h"
#include "CosmicNoiseTypes.h"
#include "CosmicNoiseSettings.h"
#include "CosmicNoiseEvaluator.h"

class FCosmicArchitectNoiseGenerator: public FNonAbandonableTask {
public:
	// Referencias a los datos inmutables de la malla
	const TArray<FVector>& BaseVertices;
	const TArray<FVector>& BaseNormals;

	// El array donde guardaremos el resultado
	TArray<FVector> CalculatedVertices;
    TArray<FLinearColor> CalculatedColors;

	// Datos de transformación
	FTransform ComponentTransform;
	FVector PlanetCenter;

    UCosmicNoiseSettings* NoiseSettings;

    FCosmicArchitectNoiseGenerator(
        const TArray<FVector>& InBaseVerts,
        const TArray<FVector>& InBaseNormals,
        FTransform InTransform,
        FVector InPlanetCenter,
        UCosmicNoiseSettings* NoiseSettings)
        : BaseVertices(InBaseVerts)
        , BaseNormals(InBaseNormals)
        , ComponentTransform(InTransform)
        , PlanetCenter(InPlanetCenter),
        NoiseSettings(NoiseSettings)
    {
        CalculatedVertices.SetNumUninitialized(BaseVertices.Num());
        CalculatedColors.SetNumUninitialized(BaseVertices.Num());
    }

    

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FCosmicArchitectNoiseGenerator, STATGROUP_ThreadPoolAsyncTasks);
	}

    void DoWork()
    {

        FCosmicNoiseEvaluator Evaluator(NoiseSettings->Params);

        const int32 VertexCount = BaseVertices.Num();

        // Loop de vértices
        for (int32 i = 0; i < VertexCount; i++)
        {
            FVector WorldPos = ComponentTransform.TransformPosition(BaseVertices[i]);
            FVector NoiseDir = (WorldPos - PlanetCenter).GetSafeNormal();

            float FinalHeight;
            FLinearColor FinalColor;

            Evaluator.EvaluatePoint(NoiseDir, FinalHeight, FinalColor);

            // Calcular posición final del vértice
            CalculatedVertices[i] = BaseVertices[i] + (BaseNormals[i] * FinalHeight);

            // Guardar colores
            CalculatedColors[i] = FinalColor;
            
        }
    }
};