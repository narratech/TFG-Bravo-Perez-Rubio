#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"
#include "ThirdParty/FastNoiseLite.h"

class FCosmicArchitectNoiseGenerator: public FNonAbandonableTask {
public:
	// Referencias a los datos inmutables de la malla
	const TArray<FVector>& BaseVertices;
	const TArray<FVector>& BaseNormals;

	// El array donde guardaremos el resultado
	TArray<FVector> CalculatedVertices;

	// Datos de transformación
	FTransform ComponentTransform;
	FVector PlanetCenter;

	// Parámetros de ruido
	float Amplitude;
	float Frequency;

	FCosmicArchitectNoiseGenerator(const TArray<FVector>& InBaseVerts, const TArray<FVector>& InBaseNormals,
		FTransform InTransform, FVector InPlanetCenter, float InAmp, float InFreq)
		: BaseVertices(InBaseVerts), BaseNormals(InBaseNormals),
		ComponentTransform(InTransform), PlanetCenter(InPlanetCenter),
		Amplitude(InAmp), Frequency(InFreq)
	{
		// Pre-reservar memoria para evitar realojamientos en el hilo
		CalculatedVertices.SetNumUninitialized(BaseVertices.Num());
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FCosmicArchitectNoiseGenerator, STATGROUP_ThreadPoolAsyncTasks);
	}

	void DoWork() {
		FastNoiseLite Noise;
		Noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

		for (int32 i = 0; i < BaseVertices.Num(); i++)
		{
			// Pasar vértice local a espacio mundo
			FVector WorldPos = ComponentTransform.TransformPosition(BaseVertices[i]);

			// Obtener dirección normalizada desde el centro del planeta (Espacio Esférico 3D)
			FVector NoiseDir = (WorldPos - PlanetCenter).GetSafeNormal();

			// Obtener ruido en 3D para evitar costuras
			float NoiseValue = Noise.GetNoise(NoiseDir.X * Frequency, NoiseDir.Y * Frequency, NoiseDir.Z * Frequency);

			// Aplicar el ruido (escala de 0 a 1)
			// FastNoise devuelve -1 a 1, lo mapeamos o lo usamos directo según el diseño
			CalculatedVertices[i] = BaseVertices[i] + (BaseNormals[i] * NoiseValue * Amplitude);
		}
	}
};