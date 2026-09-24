// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AActor;

/**
 * Manages orbital simulation state and speed parameters across generated celestial bodies.
 */
class COSMICARCHITECTRUNTIME_API FCosmicSystemOrbitController
{
public:
    /** Starts orbit simulation on all bodies containing a UCosmicOrbitComponent. */
    static void StartOrbitSimulation(const TArray<AActor*>& Bodies, float OrbitSpeedMultiplier);

    /** Stops orbit simulation on all bodies containing a UCosmicOrbitComponent. */
    static void StopOrbitSimulation(const TArray<AActor*>& Bodies);

    /** Updates the editor speed multiplier on all orbit components. */
    static void UpdateBodiesOrbitalPeriod(const TArray<AActor*>& Bodies, float OrbitSpeedMultiplier);

    /** Updates orbital components during editor tick. */
    static void TickSimulation(const TArray<AActor*>& Bodies, float OrbitSpeedMultiplier);
};
