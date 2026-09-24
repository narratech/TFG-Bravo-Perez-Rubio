// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "System/CosmicSystemTypes.h"

/**
 * Calculates orbital positions, distances, planetary classification,
 * and physical surface attributes for planetary systems.
 */
class COSMICARCHITECTRUNTIME_API FCosmicSystemLayoutManager
{
public:
    /** Checks whether a proposed orbital distance is valid against already placed bodies. */
    static bool IsOrbitDistanceValid(
        float ProposedOrbitKm,
        float ProposedRadiusKm,
        const TArray<float>& ExistingOrbits,
        const TArray<float>& ExistingRadii,
        float MinDistanceBetweenBodies
    );

    /** Tries to place a planet respecting distances, clustering, and radii factors. */
    static bool TryPlacePlanet(
        FRandomStream& Stream,
        float SystemRadiusKm,
        float StarRadiusKm,
        const TArray<float>& ExistingOrbitDistances,
        const TArray<float>& ExistingPlanetRadii,
        float& OutOrbitDistance,
        float& OutPlanetRadius,
        bool bIsGasGiant,
        const FCosmicSystemLayoutConfig& LayoutConfig,
        const FCosmicSystemClassificationRules& ClassRules
    );

    /** Classifies a proposed planetary body into Gas Giant, Asteroid Belt, or Terrestrial. */
    static FCosmicBodyClassification ClassifyPlanet(
        float OrbitDistanceKm,
        float PlanetRadiusKm,
        float SystemRadiusKm,
        FRandomStream& Stream,
        int32 RemainingBodies,
        int32 TotalBodies,
        const FCosmicSystemClassificationRules& Rules
    );

    /** Computes surface gravity interpolated between configured min and max ranges based on radius. */
    static float CalculateSurfaceGravity(
        float RadiusKm,
        const FVector2D& RadiusRangeKm,
        const FVector2D& GravityRange
    );
};
