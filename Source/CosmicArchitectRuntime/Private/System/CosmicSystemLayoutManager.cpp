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
        const float MinRadius = DiameterRange.X * 0.5f;
        const float MaxRadius = DiameterRange.Y * 0.5f;
        float Radius = Stream.FRandRange(MinRadius, MaxRadius);
        Radius = FMath::Clamp(Radius, MinRadius, MaxRadius);

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
    const float EffectiveHabitableInner = FMath::Max(Rules.HabitableZoneInnerFraction, 0.42f);
    const float EffectiveHabitableOuter = FMath::Max(Rules.HabitableZoneOuterFraction, 0.72f);
    const bool bInHabitableZone = (OrbitalFraction >= EffectiveHabitableInner &&
        OrbitalFraction <= EffectiveHabitableOuter);
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
            // Align sea level with MI_CosmicEarth2 SandMaxAltitudeKm (0.082 km) and beach/land foliage thresholds (0.070 - 0.082 km)
            Result.OceanSeaLevel = 0.07f;
            Result.Archetype = ECosmicPlanetArchetype::EarthLike;
        }
        else
        {
            Result.bHasOcean = false;
            Result.OceanSeaLevel = 0.0f;

            const float Roll = Stream.FRandRange(0.0f, 1.0f);
            if (OrbitalFraction < EffectiveHabitableInner)
            {
                // Inner warm/scorched zone: Desert Mesa, Cratered Barren, or Volcanic
                if (Roll < 0.45f)
                {
                    Result.Archetype = ECosmicPlanetArchetype::AridDesert;
                }
                else if (Roll < 0.75f)
                {
                    Result.Archetype = ECosmicPlanetArchetype::CrateredBarren;
                }
                else
                {
                    Result.Archetype = ECosmicPlanetArchetype::VolcanicScorched;
                }
            }
            else if (bInHabitableZone)
            {
                // Dry habitable zone: Arid Desert Canyon/Savanna, Frozen Highland, or Cratered
                if (Roll < 0.55f)
                {
                    Result.Archetype = ECosmicPlanetArchetype::AridDesert;
                }
                else if (Roll < 0.80f)
                {
                    Result.Archetype = ECosmicPlanetArchetype::FrozenIce;
                }
                else
                {
                    Result.Archetype = ECosmicPlanetArchetype::CrateredBarren;
                }
            }
            else
            {
                // Outer cold zone: Frozen Ice/Tundra or Cratered Barren
                Result.Archetype = (Roll < 0.55f)
                    ? ECosmicPlanetArchetype::FrozenIce
                    : ECosmicPlanetArchetype::CrateredBarren;
            }
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
