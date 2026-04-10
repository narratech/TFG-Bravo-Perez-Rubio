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

	// El array donde guardaremos el resultado
	TArray<FVector> CalculatedVertices;
    TArray<FVector> CalculatedNormals;
    TArray<FLinearColor> CalculatedColors;

	// Datos de transformación
	FTransform ComponentTransform;
	FVector PlanetCenter;

    bool IsPlanet;
    UCosmicNoiseSettings* NoiseSettings;

    FCosmicArchitectNoiseGenerator(
        const TArray<FVector>& InBaseVerts,
        FTransform InTransform,
        FVector InPlanetCenter,
        bool InPlanet,
        UCosmicNoiseSettings* NoiseSettings)
        : BaseVertices(InBaseVerts)
        , ComponentTransform(InTransform)
        , PlanetCenter(InPlanetCenter)
        ,IsPlanet(InPlanet),
        NoiseSettings(NoiseSettings)
    {
        CalculatedVertices.SetNumUninitialized(BaseVertices.Num());
        CalculatedColors.SetNumUninitialized(BaseVertices.Num());
        CalculatedNormals.SetNumUninitialized(BaseVertices.Num());
    }

    

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FCosmicArchitectNoiseGenerator, STATGROUP_ThreadPoolAsyncTasks);
	}

    void DoWork()
    {


        const int32 VertexCount = BaseVertices.Num();

        if (IsPlanet) {
            
            FMatrix TransformMatrix = ComponentTransform.ToMatrixWithScale();

            for (size_t i = 0; i < VertexCount; i++)
            {
                CalculatedVertices[i] = TransformMatrix.TransformPosition(BaseVertices[i]);
            }
        }
        
        FCosmicNoiseEvaluator Evaluator(NoiseSettings->Params);

        // Loop de vértices
        for (int32 i = 0; i < VertexCount; i++)
        {
            FVector WorldPos = IsPlanet ? CalculatedVertices[i] : BaseVertices[i];
            FVector NoiseDir = IsPlanet ? WorldPos.GetSafeNormal() : FVector(WorldPos.X, WorldPos.Y, 0);

            float FinalHeight;
            FLinearColor FinalColor;

            Evaluator.EvaluatePoint(NoiseDir, FinalHeight, FinalColor);

            // Calcular posición final del vértice
            if (IsPlanet) {
                CalculatedNormals[i] = NoiseDir;
                CalculatedVertices[i] += (NoiseDir * FinalHeight);
            }
            else {
                CalculatedVertices[i] = BaseVertices[i] + (FVector::UpVector * FinalHeight);
            }
            

            // Guardar colores
            CalculatedColors[i] = FinalColor;
            
        }
    }
};