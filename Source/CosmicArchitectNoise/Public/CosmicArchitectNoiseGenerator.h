#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"
#include "ThirdParty/FastNoiseLite.h"

class FCosmicArchitectNoiseGenerator: public FNonAbandonableTask {
public:
	TArray<float>& OutHeights;
	FVector Offset;
	int32 Size;
	float Scale;

	FCosmicArchitectNoiseGenerator(TArray<float>& InOutHeights, FVector InOffset, int32 InSize, float InScale)
		: OutHeights(InOutHeights), Offset(InOffset), Size(InSize), Scale(InScale)
	{
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FPlanetNoiseWorker, STATGROUP_ThreadPoolAsyncTasks);
	}

	void DoWork() {
        FastNoiseLite Noise;
        Noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        Noise.SetSeed(1337); // O un seed que pases por el constructor

        for (int32 y = 0; y < Size; y++)
        {
            for (int32 x = 0; x < Size; x++)
            {
                // Lógica de muestreo esférico para evitar distorsión en planetas
                FVector LocalPos = Offset + FVector(x, y, 0);
                FVector NormalPos = LocalPos.GetSafeNormal();

                // Guardamos el ruido en el array referenciado
                OutHeights[x + y * Size] = Noise.GetNoise(
                    (float)(NormalPos.X * Scale),
                    (float)(NormalPos.Y * Scale),
                    (float)(NormalPos.Z * Scale)
                );
            }
        }
	}
};