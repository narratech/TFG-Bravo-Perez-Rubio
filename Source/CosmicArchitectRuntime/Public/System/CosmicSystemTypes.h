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
 * Planet appearance and manual planetary material parameter pack.
 * Defaults match the Earth-Like Whittaker biome & sedimentary strata configuration.
 */
struct FCosmicPlanetAppearanceParams
{
    // 8 Analytical Whittaker Biome Colors (Earth-like defaults)
    FLinearColor BiomeColorPolarIce   = FLinearColor(0.88f, 0.92f, 0.96f, 1.0f);
    FLinearColor BiomeColorTundra     = FLinearColor(0.42f, 0.40f, 0.32f, 1.0f);
    FLinearColor BiomeColorTaiga      = FLinearColor(0.12f, 0.22f, 0.10f, 1.0f);
    FLinearColor BiomeColorGrassland  = FLinearColor(0.32f, 0.38f, 0.16f, 1.0f);
    FLinearColor BiomeColorForest     = FLinearColor(0.14f, 0.30f, 0.09f, 1.0f);
    FLinearColor BiomeColorDesert     = FLinearColor(0.76f, 0.58f, 0.35f, 1.0f);
    FLinearColor BiomeColorSavannah   = FLinearColor(0.55f, 0.48f, 0.22f, 1.0f);
    FLinearColor BiomeColorRainforest = FLinearColor(0.06f, 0.20f, 0.05f, 1.0f);

    // Geological Sedimentary Strata (Earth-like defaults)
    FLinearColor StrataColor1 = FLinearColor(0.58f, 0.45f, 0.32f, 1.0f);
    FLinearColor StrataColor2 = FLinearColor(0.42f, 0.25f, 0.18f, 1.0f);
    FLinearColor StrataColor3 = FLinearColor(0.22f, 0.20f, 0.22f, 1.0f);
    float StrataFrequency     = 0.08f;
    float StrataDistortion    = 0.45f;
    float StrataNormalStrength = 0.35f;

    // Snow, Wetness & Magma (Earth-like defaults)
    bool bEnableSnow              = true;
    FLinearColor SnowColor        = FLinearColor(0.92f, 0.94f, 0.98f, 1.0f);
    float SnowTemperatureThreshold = 0.22f;
    float WetnessIntensity        = 1.0f;
    float MagmaIntensity          = 0.0f;

    // Terrain Relief & Cliffs (Earth-like defaults)
    float MacroFrequency      = 0.00003f;
    float MicroFrequency      = 0.000002f;
    float NormalStrength      = 1.0f;
    float SlopeCliffThreshold = 0.15f;
    float SlopeCliffFallOff   = 0.20f;

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
        Params.bEnableSnow = Stream.FRandRange(0.0f, 1.0f) < 0.5f;
        Params.WetnessIntensity = Stream.FRandRange(0.0f, 1.0f);
        Params.MagmaIntensity = (Stream.FRandRange(0.0f, 1.0f) < 0.15f) ? Stream.FRandRange(0.5f, 1.5f) : 0.0f;

        const FLinearColor Cold = FLinearColor::FromSRGBColor(GetRandomColor(Stream, 50, 255));
        const FLinearColor Mid  = FLinearColor::FromSRGBColor(GetRandomColor(Stream, 50, 255));
        const FLinearColor Warm = FLinearColor::FromSRGBColor(GetRandomColor(Stream, 50, 255));
        const FLinearColor Rock = FLinearColor::FromSRGBColor(GetRandomColor(Stream, 50, 200));

        Params.BiomeColorPolarIce   = FMath::Lerp(Cold, FLinearColor(0.9f, 0.94f, 0.98f, 1.0f), 0.5f);
        Params.BiomeColorTundra     = Cold;
        Params.BiomeColorTaiga      = FMath::Lerp(Cold, Mid, 0.45f);
        Params.BiomeColorGrassland  = Mid;
        Params.BiomeColorForest     = Mid * 0.75f;
        Params.BiomeColorDesert     = Warm;
        Params.BiomeColorSavannah   = FMath::Lerp(Mid, Warm, 0.55f);
        Params.BiomeColorRainforest = FMath::Lerp(Mid, Warm, 0.25f) * 0.65f;

        Params.StrataColor1 = Rock * 1.25f;
        Params.StrataColor2 = Rock;
        Params.StrataColor3 = Rock * 0.70f;

        return Params;
    }

    static FCosmicPlanetAppearanceParams MakeMoonAppearance(FRandomStream& Stream)
    {
        FCosmicPlanetAppearanceParams Params;
        Params.bEnableSnow = false;
        Params.WetnessIntensity = 0.0f;
        Params.MagmaIntensity = 0.0f;

        const FLinearColor Low  = FLinearColor::FromSRGBColor(GetRandomColor(Stream, 50, 180));
        const FLinearColor Mid  = FLinearColor::FromSRGBColor(GetRandomColor(Stream, 70, 200));
        const FLinearColor High = FLinearColor::FromSRGBColor(GetRandomColor(Stream, 90, 220));
        const FLinearColor Rock = FLinearColor::FromSRGBColor(GetRandomColor(Stream, 60, 170));

        Params.BiomeColorPolarIce   = High;
        Params.BiomeColorTundra     = Low;
        Params.BiomeColorTaiga      = FMath::Lerp(Low, Mid, 0.4f);
        Params.BiomeColorGrassland  = Mid;
        Params.BiomeColorForest     = Mid * 0.85f;
        Params.BiomeColorDesert     = High;
        Params.BiomeColorSavannah   = FMath::Lerp(Mid, High, 0.5f);
        Params.BiomeColorRainforest = Low * 0.85f;

        Params.StrataColor1 = Rock * 1.2f;
        Params.StrataColor2 = Rock;
        Params.StrataColor3 = Rock * 0.65f;

        return Params;
    }

    static FCosmicPlanetAppearanceParams MakeStarAppearance(FRandomStream& Stream)
    {
        FCosmicPlanetAppearanceParams Params;
        Params.bEnableSnow = false;
        Params.WetnessIntensity = 0.0f;
        Params.MagmaIntensity = 1.0f;

        const FLinearColor Hot  = FLinearColor::FromSRGBColor(GetRandomColor(Stream, 180, 255));
        const FLinearColor Warm = FLinearColor::FromSRGBColor(GetRandomColor(Stream, 100, 220));
        const FLinearColor Cool = FLinearColor::FromSRGBColor(GetRandomColor(Stream, 50, 180));

        Params.BiomeColorPolarIce   = Cool;
        Params.BiomeColorTundra     = Cool;
        Params.BiomeColorTaiga      = Warm;
        Params.BiomeColorGrassland  = Warm;
        Params.BiomeColorForest     = Warm;
        Params.BiomeColorDesert     = Hot;
        Params.BiomeColorSavannah   = Hot;
        Params.BiomeColorRainforest = Hot;

        Params.StrataColor1 = Hot;
        Params.StrataColor2 = Warm;
        Params.StrataColor3 = Cool;

        return Params;
    }
};
