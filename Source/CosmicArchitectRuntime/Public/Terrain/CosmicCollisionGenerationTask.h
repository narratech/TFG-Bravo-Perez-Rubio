// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"

class ICosmicNoiseStrategy;

/**
 * Asynchronous task to calculate deformed collision vertices
 * on a background worker thread using procedural noise.
 */
class COSMICARCHITECTRUNTIME_API FCosmicCollisionGenerationTask : public FNonAbandonableTask
{
public:

    /** Base mesh vertices without deformation */
    TArray<FVector> BaseVertices;

    /** Base normals used for radial displacement */
    TArray<FVector> BaseNormals;

    /** Target world transform of the collision patch */
    FTransform PatchTransform;

    /** Global center of the planet */
    FVector PlanetCenter;

    /** Procedural noise strategy */
    TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy;

    /** Resulting deformed vertices in local patch space */
    TArray<FVector> CalculatedVertices;

    /**
     * Constructor for collision deformation task.
     */
    FCosmicCollisionGenerationTask(
        const TArray<FVector>& InBaseVerts,
        const TArray<FVector>& InBaseNormals,
        const FTransform& InPatchTransform,
        const FVector& InPlanetCenter,
        TSharedPtr<ICosmicNoiseStrategy> InNoiseStrategy
    );

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(FCosmicCollisionGenerationTask, STATGROUP_ThreadPoolAsyncTasks);
    }

    /**
     * Evaluates noise on a background worker thread.
     */
    void DoWork();
};
