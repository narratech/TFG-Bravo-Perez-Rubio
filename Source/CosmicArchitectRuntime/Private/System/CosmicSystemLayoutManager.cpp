// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "System/CosmicSystemLayoutManager.h"
#include "Math/RandomStream.h"
#include "Math/UnrealMathUtility.h"

bool FCosmicSystemLayoutManager::IsOrbitDistanceValid(
    float ProposedOrbitKm,
    float ProposedRadiusKm,
    const TArray<float>& ExistingOrbits,
    const TArray<float>& ExistingRadii,
    float MinDistanceBetweenBodies)
{
    for (int32 i = 0; i < ExistingOrbits.Num(); ++i)
    {
        const float RequiredSeparation = ProposedRadiusKm + ExistingRadii[i] + MinDistanceBetweenBodies;
        if (FMath::Abs(ProposedOrbitKm - ExistingOrbits[i]) < RequiredSeparation)
        {
            return false;
        }
    }
    return true;
}

bool FCosmicSystemLayoutManager::TryPlacePlanet(
    FRandomStream& Stream,
    float SystemRadiusKm,
    float StarRadiusKm,
    const TArray<float>& ExistingOrbitDistances,
    const TArray<float>& ExistingPlanetRadii,
    float& OutOrbitDistance,
    float& OutPlanetRadius,
    bool bIsGasGiant,
    const FCosmicSystemLayoutConfig& LayoutConfig,
    const FCosmicSystemClassificationRules& ClassRules)
{
    const float MinDist = StarRadiusKm * LayoutConfig.OrbitDistanceMinFactor;
    const float MaxDist = SystemRadiusKm;

    const float RadiusFactorMin = bIsGasGiant ? ClassRules.GasGiantRadiusFactorMin : LayoutConfig.PlanetRadiusFactorMin;
    const float RadiusFactorMax = bIsGasGiant ? ClassRules.GasGiantRadiusFactorMax : LayoutConfig.PlanetRadiusFactorMax;

    const FVector2D& DiameterRange = LayoutConfig.BodyDiameterRangeKm;

    for (int32 Attempt = 0; Attempt < LayoutConfig.MaxGenerationAttempts; ++Attempt)
    {
        const float Orbit = Stream.FRandRange(MinDist, MaxDist);
        float Radius = Orbit * Stream.FRandRange(RadiusFactorMin, RadiusFactorMax);

        Radius = FMath::Clamp(Radius, DiameterRange.X * 0.5f, DiameterRange.Y * 0.5f);

        if (!IsOrbitDistanceValid(Orbit, Radius, ExistingOrbitDistances, ExistingPlanetRadii, LayoutConfig.MinDistanceBetweenBodies))
        {
            continue;
        }

        if (LayoutConfig.MaxDistanceToNearest > 0.0f && ExistingOrbitDistances.Num() > 0)
        {
            bool bHasNeighbor = false;
            for (float ExistingOrbit : ExistingOrbitDistances)
            {
                if (FMath::Abs(Orbit - ExistingOrbit) <= LayoutConfig.MaxDistanceToNearest)
                {
                    bHasNeighbor = true;
                    break;
                }
            }
            if (!bHasNeighbor)
            {
                continue;
            }
        }

        OutOrbitDistance = Orbit;
        OutPlanetRadius = Radius;
        return true;
    }

    return false;
}

FCosmicBodyClassification FCosmicSystemLayoutManager::ClassifyPlanet(
    float OrbitDistanceKm,
    float PlanetRadiusKm,
    float SystemRadiusKm,
    FRandomStream& Stream,
    int32 RemainingBodies,
    int32 TotalBodies,
    const FCosmicSystemClassificationRules& Rules)
{
    FCosmicBodyClassification Result;

    const float OrbitalFraction = OrbitDistanceKm / SystemRadiusKm;
    const bool bInHabitableZone = (OrbitalFraction >= Rules.HabitableZoneInnerFraction &&
        OrbitalFraction <= Rules.HabitableZoneOuterFraction);
    const bool bInBeltZone = (OrbitalFraction >= Rules.BeltZoneInnerFraction &&
        OrbitalFraction <= Rules.BeltZoneOuterFraction);

    const float RemainingFraction = (float)RemainingBodies / FMath::Max(1, TotalBodies);

    if (RemainingFraction <= Rules.GasGiantAppearanceThreshold &&
        Stream.FRandRange(0.0f, 1.0f) < Rules.GasGiantProbability)
    {
        Result.Type = ECosmicSystemPlanetType::GasGiant;
        Result.bHasOcean = false;
        Result.bHasRings = Stream.FRandRange(0.0f, 1.0f) < Rules.GasGiantRingProbability;
        Result.bHasMoons = true;
        Result.MaxMoons = Stream.RandRange(Rules.GasGiantMoonMin, Rules.GasGiantMoonMax);
    }
    else if (bInBeltZone && Stream.FRandRange(0.0f, 1.0f) < Rules.BeltProbability)
    {
        Result.Type = ECosmicSystemPlanetType::AsteroidBelt;
        Result.bHasOcean = false;
        Result.bHasRings = false;
        Result.bHasMoons = false;
        Result.MaxMoons = 0;
    }
    else
    {
        Result.Type = ECosmicSystemPlanetType::Telluric;
        Result.bHasRings = false;
        Result.bHasMoons = true;
        Result.MaxMoons = Stream.RandRange(Rules.TelluricMoonMin, Rules.TelluricMoonMax);

        if (bInHabitableZone && Stream.FRandRange(0.0f, 1.0f) < Rules.TelluricOceanProbability)
        {
            Result.bHasOcean = true;
            Result.OceanSeaLevel = Stream.FRandRange(-0.00002f, 0.00002f) * PlanetRadiusKm;
        }
        else
        {
            Result.bHasOcean = false;
        }
    }

    return Result;
}

float FCosmicSystemLayoutManager::CalculateSurfaceGravity(
    float RadiusKm,
    const FVector2D& RadiusRangeKm,
    const FVector2D& GravityRange)
{
    const float MinRadius = FMath::Min(RadiusRangeKm.X, RadiusRangeKm.Y);
    const float MaxRadius = FMath::Max(RadiusRangeKm.X, RadiusRangeKm.Y);
    const float MinGravity = FMath::Min(GravityRange.X, GravityRange.Y);
    const float MaxGravity = FMath::Max(GravityRange.X, GravityRange.Y);

    if (FMath::IsNearlyEqual(MinRadius, MaxRadius))
    {
        return MinGravity;
    }

    const float RadiusAlpha = FMath::Clamp((RadiusKm - MinRadius) / (MaxRadius - MinRadius), 0.0f, 1.0f);
    return FMath::Lerp(MinGravity, MaxGravity, RadiusAlpha);
}
