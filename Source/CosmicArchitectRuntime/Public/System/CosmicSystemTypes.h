// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Classification of planetary bodies in the generated system.
 */
enum class ECosmicSystemPlanetType : uint8
{
    GasGiant,
    Telluric,
    AsteroidBelt
};

/**
 * Procedural classification result for a proposed celestial body.
 */
struct FCosmicBodyClassification
{
    ECosmicSystemPlanetType Type = ECosmicSystemPlanetType::Telluric;
    bool bHasOcean = false;
    float OceanSeaLevel = 0.0f;
    bool bHasRings = false;
    bool bHasMoons = false;
    int32 MaxMoons = 0;
};

/**
 * Configuration rules for celestial body orbital placement and sizes.
 */
struct FCosmicSystemLayoutConfig
{
    FVector VolumeSizeKm = FVector(3000.0f, 3000.0f, 5.0f);
    FVector2D BodyDiameterRangeKm = FVector2D(8.0f, 15.0f);
    FVector2D MoonDiameterRangeKm = FVector2D(2.0f, 7.0f);
    FVector2D PlanetSurfaceGravityRange = FVector2D(3.0f, 10.0f);
    FVector2D MoonSurfaceGravityRange = FVector2D(1.0f, 5.0f);
    float StarRadiusFraction = 0.1f;
    float MinDistanceBetweenBodies = 5.0f;
    float MaxDistanceToNearest = 0.0f;
    int32 MaxGenerationAttempts = 100;
    float OrbitDistanceMinFactor = 3.0f;
    float PlanetRadiusFactorMin = 0.01f;
    float PlanetRadiusFactorMax = 0.06f;
    float MoonOrbitDistanceFactorMin = 10.0f;
    float MoonOrbitDistanceFactorMax = 15.0f;
    float MoonRadiusFactorMin = 0.1f;
    float MoonRadiusFactorMax = 0.3f;
};

/**
 * Probability and zone thresholds for body classification.
 */
struct FCosmicSystemClassificationRules
{
    float GasGiantAppearanceThreshold = 0.3f;
    float GasGiantProbability = 0.7f;
    float GasGiantRadiusFactorMin = 0.001f;
    float GasGiantRadiusFactorMax = 0.5f;
    float GasGiantRadiusMin = 30.0f;
    float GasGiantRadiusMax = 50.0f;
    float HabitableZoneInnerFraction = 0.25f;
    float HabitableZoneOuterFraction = 0.65f;
    float BeltZoneInnerFraction = 0.55f;
    float BeltZoneOuterFraction = 0.70f;
    float BeltProbability = 0.6f;
    float GasGiantRingProbability = 0.65f;
    float TelluricOceanProbability = 0.7f;
    int32 GasGiantMoonMin = 1;
    int32 GasGiantMoonMax = 6;
    int32 TelluricMoonMin = 0;
    int32 TelluricMoonMax = 3;
};

/**
 * Graphic and clipmap resolutions for generated bodies.
 */
struct FCosmicSystemGraphicsConfig
{
    int32 GasGiantClipResolution = 64;
    int32 TelluricClipResolution = 128;
    int32 OceanResolutionWithOcean = 128;
    int32 OceanResolutionWithoutOcean = 64;
};

/**
 * Planet appearance and color parameter pack.
 * Encapsulates both modern archetype-based shaders and legacy color parameters
 * to shield the generator from shader parameter refactoring in other modules.
 */
struct FCosmicPlanetAppearanceParams
{
    int32 ArchetypeIndex = 0;
    bool bUseCustomArchetype = true;
    FLinearColor TerrainColorLow = FLinearColor(0.212f, 0.028f, 0.026f, 1.0f);
    FLinearColor TerrainColorMid = FLinearColor(0.509f, 0.014f, 0.008f, 1.0f);
    FLinearColor TerrainColorHigh = FLinearColor(0.723f, 0.168f, 0.012f, 1.0f);
    FLinearColor RockColor = FLinearColor(0.799f, 0.397f, 0.171f, 1.0f);
    bool bEnableSnow = true;

    // Legacy parameters maintained for backward compatibility
    FColor LegacyColor1 = FColor::Red;
    FColor LegacyColor2 = FColor::Orange;
    FColor LegacyColorCold = FColor::White;
    FColor LegacyColorHot = FColor::Red;
    FColor LegacyColorSlope = FColor::Black;
    float NoiseScaleSmall = 1.0f;
    float NoiseScaleMedium = 3.0f;
    float NoiseScaleLarge = 100.0f;

    static FColor GetRandomColor(FRandomStream& Stream, int32 Min, int32 Max)
    {
        const int32 MinRange = FMath::Max(Min, 0);
        const int32 MaxRange = FMath::Min(Max, 255);
        return FColor(
            Stream.RandRange(MinRange, MaxRange),
            Stream.RandRange(MinRange, MaxRange),
            Stream.RandRange(MinRange, MaxRange),
            255
        );
    }

    static FCosmicPlanetAppearanceParams MakeTelluricAppearance(FRandomStream& Stream)
    {
        FCosmicPlanetAppearanceParams Params;
        Params.ArchetypeIndex = Stream.RandRange(0, 3);
        Params.bUseCustomArchetype = true;
        Params.bEnableSnow = Stream.FRandRange(0.0f, 1.0f) < 0.5f;

        Params.LegacyColor1 = GetRandomColor(Stream, 50, 255);
        Params.LegacyColor2 = GetRandomColor(Stream, 50, 255);
        Params.LegacyColorCold = GetRandomColor(Stream, 50, 255);
        Params.LegacyColorHot = GetRandomColor(Stream, 50, 255);
        Params.LegacyColorSlope = GetRandomColor(Stream, 50, 255);
        Params.NoiseScaleSmall = Stream.FRandRange(0.5f, 2.0f);
        Params.NoiseScaleMedium = Stream.FRandRange(3.0f, 5.0f);
        Params.NoiseScaleLarge = Stream.FRandRange(50.0f, 100.0f);

        Params.TerrainColorLow = FLinearColor::FromSRGBColor(Params.LegacyColorCold);
        Params.TerrainColorMid = FLinearColor::FromSRGBColor(Params.LegacyColor1);
        Params.TerrainColorHigh = FLinearColor::FromSRGBColor(Params.LegacyColor2);
        Params.RockColor = FLinearColor::FromSRGBColor(Params.LegacyColorSlope);

        return Params;
    }

    static FCosmicPlanetAppearanceParams MakeMoonAppearance(FRandomStream& Stream)
    {
        FCosmicPlanetAppearanceParams Params;
        Params.ArchetypeIndex = 2; // Ice / Rocky moon
        Params.bUseCustomArchetype = true;
        Params.bEnableSnow = false;

        Params.LegacyColor1 = GetRandomColor(Stream, 50, 200);
        Params.LegacyColor2 = GetRandomColor(Stream, 50, 200);
        Params.LegacyColorCold = GetRandomColor(Stream, 50, 200);
        Params.LegacyColorHot = GetRandomColor(Stream, 50, 200);
        Params.LegacyColorSlope = GetRandomColor(Stream, 50, 200);
        Params.NoiseScaleSmall = Stream.FRandRange(0.5f, 2.0f);
        Params.NoiseScaleMedium = Stream.FRandRange(3.0f, 5.0f);
        Params.NoiseScaleLarge = Stream.FRandRange(50.0f, 100.0f);

        Params.TerrainColorLow = FLinearColor::FromSRGBColor(Params.LegacyColorCold);
        Params.TerrainColorMid = FLinearColor::FromSRGBColor(Params.LegacyColor1);
        Params.TerrainColorHigh = FLinearColor::FromSRGBColor(Params.LegacyColor2);
        Params.RockColor = FLinearColor::FromSRGBColor(Params.LegacyColorSlope);

        return Params;
    }

    static FCosmicPlanetAppearanceParams MakeStarAppearance(FRandomStream& Stream)
    {
        FCosmicPlanetAppearanceParams Params;
        Params.ArchetypeIndex = 3;
        Params.bUseCustomArchetype = true;
        Params.bEnableSnow = false;

        Params.LegacyColor1 = GetRandomColor(Stream, 180, 255);
        Params.LegacyColor2 = GetRandomColor(Stream, 100, 220);
        Params.LegacyColorCold = GetRandomColor(Stream, 50, 180);
        Params.LegacyColorHot = GetRandomColor(Stream, 50, 180);
        Params.LegacyColorSlope = GetRandomColor(Stream, 50, 180);
        Params.NoiseScaleSmall = Stream.FRandRange(0.5f, 2.0f);
        Params.NoiseScaleMedium = Stream.FRandRange(4.0f, 8.0f);
        Params.NoiseScaleLarge = Stream.FRandRange(4.0f, 6.0f);

        Params.TerrainColorLow = FLinearColor::FromSRGBColor(Params.LegacyColorCold);
        Params.TerrainColorMid = FLinearColor::FromSRGBColor(Params.LegacyColor1);
        Params.TerrainColorHigh = FLinearColor::FromSRGBColor(Params.LegacyColor2);
        Params.RockColor = FLinearColor::FromSRGBColor(Params.LegacyColorSlope);

        return Params;
    }
};
