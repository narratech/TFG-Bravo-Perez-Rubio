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
 * Specific surface & climate archetype for terrestrial planets and moons.
 * Drives the selection of noise strategy (MultiNoise, ErosionMultifractal, Crater),
 * curated material palettes, and biome-matched FoliageCollections.
 */
enum class ECosmicPlanetArchetype : uint8
{
    EarthLike,
    AridDesert,
    FrozenIce,
    CrateredBarren,
    VolcanicScorched,
    Moon
};

/**
 * Procedural classification result for a proposed celestial body.
 */
struct FCosmicBodyClassification
{
    ECosmicSystemPlanetType Type = ECosmicSystemPlanetType::Telluric;
    ECosmicPlanetArchetype Archetype = ECosmicPlanetArchetype::EarthLike;
    bool bHasOcean = false;
    float OceanSeaLevel = 0.07f;
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
 * Defaults match the Earth-Like Whittaker biome & sedimentary strata configuration in EarthPlanet.
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
    float StrataFrequency      = 0.08f;
    float StrataDistortion     = 0.45f;
    float StrataNormalStrength = 0.35f;

    // Snow, Wetness & Magma (Earth-like defaults)
    bool bEnableSnow               = true;
    FLinearColor SnowColor         = FLinearColor(0.92f, 0.94f, 0.98f, 1.0f);
    float SnowTemperatureThreshold = 0.22f;
    float WetnessIntensity         = 1.0f;
    float MagmaIntensity           = 0.0f;

    // Terrain Relief & Cliffs (Matched to MI_CosmicEarth2 in EarthPlanet)
    float MacroFrequency      = 0.00003f;
    float MicroFrequency      = 0.000002f;
    float NormalStrength      = 1.0f;
    float SlopeCliffThreshold = 0.08f;
    float SlopeCliffFallOff   = 0.45f;

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

    static FLinearColor JitterColor(FRandomStream& Stream, const FLinearColor& Base, float Range = 0.04f)
    {
        const float Scale = Stream.FRandRange(1.0f - Range, 1.0f + Range);
        return FLinearColor(
            FMath::Clamp(Base.R * Scale + Stream.FRandRange(-Range * 0.5f, Range * 0.5f), 0.01f, 1.0f),
            FMath::Clamp(Base.G * Scale + Stream.FRandRange(-Range * 0.5f, Range * 0.5f), 0.01f, 1.0f),
            FMath::Clamp(Base.B * Scale + Stream.FRandRange(-Range * 0.5f, Range * 0.5f), 0.01f, 1.0f),
            1.0f
        );
    }

    static FCosmicPlanetAppearanceParams MakeAppearanceForArchetype(ECosmicPlanetArchetype Archetype, FRandomStream& Stream)
    {
        FCosmicPlanetAppearanceParams Params;

        switch (Archetype)
        {
        case ECosmicPlanetArchetype::EarthLike:
        {
            const int32 SubPalette = Stream.RandRange(0, 2);
            Params.bEnableSnow = true;
            Params.SnowTemperatureThreshold = Stream.FRandRange(0.20f, 0.24f);
            Params.WetnessIntensity = Stream.FRandRange(0.90f, 1.15f);
            Params.MagmaIntensity = 0.0f;
            Params.SlopeCliffThreshold = 0.08f;
            Params.SlopeCliffFallOff = 0.48f;

            if (SubPalette == 0)
            {
                // Classic Earth-Like (matches EarthPlanet closely)
                Params.BiomeColorPolarIce   = JitterColor(Stream, FLinearColor(0.88f, 0.92f, 0.96f, 1.0f), 0.03f);
                Params.BiomeColorTundra     = JitterColor(Stream, FLinearColor(0.42f, 0.40f, 0.32f, 1.0f), 0.04f);
                Params.BiomeColorTaiga      = JitterColor(Stream, FLinearColor(0.12f, 0.22f, 0.10f, 1.0f), 0.03f);
                Params.BiomeColorGrassland  = JitterColor(Stream, FLinearColor(0.32f, 0.38f, 0.16f, 1.0f), 0.04f);
                Params.BiomeColorForest     = JitterColor(Stream, FLinearColor(0.14f, 0.30f, 0.09f, 1.0f), 0.03f);
                Params.BiomeColorDesert     = JitterColor(Stream, FLinearColor(0.76f, 0.58f, 0.35f, 1.0f), 0.04f);
                Params.BiomeColorSavannah   = JitterColor(Stream, FLinearColor(0.55f, 0.48f, 0.22f, 1.0f), 0.04f);
                Params.BiomeColorRainforest = JitterColor(Stream, FLinearColor(0.06f, 0.20f, 0.05f, 1.0f), 0.03f);
            }
            else if (SubPalette == 1)
            {
                // Lush Emerald Tropical Gaia
                Params.BiomeColorPolarIce   = JitterColor(Stream, FLinearColor(0.90f, 0.95f, 0.98f, 1.0f), 0.02f);
                Params.BiomeColorTundra     = JitterColor(Stream, FLinearColor(0.36f, 0.42f, 0.32f, 1.0f), 0.04f);
                Params.BiomeColorTaiga      = JitterColor(Stream, FLinearColor(0.09f, 0.24f, 0.12f, 1.0f), 0.03f);
                Params.BiomeColorGrassland  = JitterColor(Stream, FLinearColor(0.24f, 0.42f, 0.15f, 1.0f), 0.04f);
                Params.BiomeColorForest     = JitterColor(Stream, FLinearColor(0.10f, 0.32f, 0.11f, 1.0f), 0.03f);
                Params.BiomeColorDesert     = JitterColor(Stream, FLinearColor(0.80f, 0.64f, 0.40f, 1.0f), 0.04f);
                Params.BiomeColorSavannah   = JitterColor(Stream, FLinearColor(0.46f, 0.48f, 0.20f, 1.0f), 0.04f);
                Params.BiomeColorRainforest = JitterColor(Stream, FLinearColor(0.04f, 0.22f, 0.06f, 1.0f), 0.03f);
            }
            else
            {
                // Autumnal / Golden Temperate Continental
                Params.BiomeColorPolarIce   = JitterColor(Stream, FLinearColor(0.89f, 0.92f, 0.95f, 1.0f), 0.03f);
                Params.BiomeColorTundra     = JitterColor(Stream, FLinearColor(0.45f, 0.38f, 0.28f, 1.0f), 0.04f);
                Params.BiomeColorTaiga      = JitterColor(Stream, FLinearColor(0.14f, 0.23f, 0.10f, 1.0f), 0.03f);
                Params.BiomeColorGrassland  = JitterColor(Stream, FLinearColor(0.40f, 0.40f, 0.16f, 1.0f), 0.04f);
                Params.BiomeColorForest     = JitterColor(Stream, FLinearColor(0.22f, 0.31f, 0.09f, 1.0f), 0.04f);
                Params.BiomeColorDesert     = JitterColor(Stream, FLinearColor(0.74f, 0.54f, 0.32f, 1.0f), 0.04f);
                Params.BiomeColorSavannah   = JitterColor(Stream, FLinearColor(0.60f, 0.46f, 0.20f, 1.0f), 0.04f);
                Params.BiomeColorRainforest = JitterColor(Stream, FLinearColor(0.09f, 0.22f, 0.06f, 1.0f), 0.03f);
            }

            Params.StrataColor1 = JitterColor(Stream, FLinearColor(0.58f, 0.45f, 0.32f, 1.0f), 0.04f);
            Params.StrataColor2 = JitterColor(Stream, FLinearColor(0.42f, 0.25f, 0.18f, 1.0f), 0.04f);
            Params.StrataColor3 = JitterColor(Stream, FLinearColor(0.22f, 0.20f, 0.22f, 1.0f), 0.03f);
            break;
        }

        case ECosmicPlanetArchetype::AridDesert:
        {
            const int32 SubPalette = Stream.RandRange(0, 1);
            Params.bEnableSnow = false;
            Params.WetnessIntensity = Stream.FRandRange(0.0f, 0.15f);
            Params.MagmaIntensity = 0.0f;
            Params.StrataFrequency = Stream.FRandRange(0.09f, 0.14f);
            Params.StrataDistortion = Stream.FRandRange(0.45f, 0.65f);
            Params.StrataNormalStrength = Stream.FRandRange(0.45f, 0.65f);
            Params.SlopeCliffThreshold = 0.07f;
            Params.SlopeCliffFallOff = 0.35f;

            if (SubPalette == 0)
            {
                // Canyon Sandstone & Red Mesa Desert
                Params.BiomeColorPolarIce   = JitterColor(Stream, FLinearColor(0.76f, 0.58f, 0.42f, 1.0f), 0.04f);
                Params.BiomeColorTundra     = JitterColor(Stream, FLinearColor(0.58f, 0.38f, 0.24f, 1.0f), 0.04f);
                Params.BiomeColorTaiga      = JitterColor(Stream, FLinearColor(0.48f, 0.34f, 0.20f, 1.0f), 0.04f);
                Params.BiomeColorGrassland  = JitterColor(Stream, FLinearColor(0.62f, 0.46f, 0.26f, 1.0f), 0.04f);
                Params.BiomeColorForest     = JitterColor(Stream, FLinearColor(0.42f, 0.36f, 0.18f, 1.0f), 0.04f);
                Params.BiomeColorDesert     = JitterColor(Stream, FLinearColor(0.82f, 0.56f, 0.32f, 1.0f), 0.04f);
                Params.BiomeColorSavannah   = JitterColor(Stream, FLinearColor(0.68f, 0.48f, 0.25f, 1.0f), 0.04f);
                Params.BiomeColorRainforest = JitterColor(Stream, FLinearColor(0.36f, 0.34f, 0.16f, 1.0f), 0.04f);

                Params.StrataColor1 = JitterColor(Stream, FLinearColor(0.75f, 0.46f, 0.26f, 1.0f), 0.04f);
                Params.StrataColor2 = JitterColor(Stream, FLinearColor(0.54f, 0.26f, 0.15f, 1.0f), 0.04f);
                Params.StrataColor3 = JitterColor(Stream, FLinearColor(0.32f, 0.20f, 0.16f, 1.0f), 0.03f);
            }
            else
            {
                // Golden Dune & Savanna Badlands
                Params.BiomeColorPolarIce   = JitterColor(Stream, FLinearColor(0.84f, 0.74f, 0.52f, 1.0f), 0.04f);
                Params.BiomeColorTundra     = JitterColor(Stream, FLinearColor(0.56f, 0.48f, 0.32f, 1.0f), 0.04f);
                Params.BiomeColorTaiga      = JitterColor(Stream, FLinearColor(0.42f, 0.40f, 0.22f, 1.0f), 0.04f);
                Params.BiomeColorGrassland  = JitterColor(Stream, FLinearColor(0.54f, 0.48f, 0.24f, 1.0f), 0.04f);
                Params.BiomeColorForest     = JitterColor(Stream, FLinearColor(0.34f, 0.36f, 0.16f, 1.0f), 0.04f);
                Params.BiomeColorDesert     = JitterColor(Stream, FLinearColor(0.85f, 0.68f, 0.40f, 1.0f), 0.04f);
                Params.BiomeColorSavannah   = JitterColor(Stream, FLinearColor(0.64f, 0.54f, 0.28f, 1.0f), 0.04f);
                Params.BiomeColorRainforest = JitterColor(Stream, FLinearColor(0.28f, 0.32f, 0.14f, 1.0f), 0.04f);

                Params.StrataColor1 = JitterColor(Stream, FLinearColor(0.68f, 0.54f, 0.34f, 1.0f), 0.04f);
                Params.StrataColor2 = JitterColor(Stream, FLinearColor(0.48f, 0.34f, 0.22f, 1.0f), 0.04f);
                Params.StrataColor3 = JitterColor(Stream, FLinearColor(0.26f, 0.22f, 0.18f, 1.0f), 0.03f);
            }
            break;
        }

        case ECosmicPlanetArchetype::FrozenIce:
        {
            Params.bEnableSnow = true;
            Params.SnowColor = JitterColor(Stream, FLinearColor(0.90f, 0.95f, 1.0f, 1.0f), 0.02f);
            Params.SnowTemperatureThreshold = Stream.FRandRange(0.36f, 0.50f);
            Params.WetnessIntensity = Stream.FRandRange(0.40f, 0.75f);
            Params.MagmaIntensity = 0.0f;
            Params.SlopeCliffThreshold = 0.08f;
            Params.SlopeCliffFallOff = 0.40f;

            Params.BiomeColorPolarIce   = JitterColor(Stream, FLinearColor(0.88f, 0.94f, 0.99f, 1.0f), 0.02f);
            Params.BiomeColorTundra     = JitterColor(Stream, FLinearColor(0.46f, 0.48f, 0.44f, 1.0f), 0.04f);
            Params.BiomeColorTaiga      = JitterColor(Stream, FLinearColor(0.12f, 0.22f, 0.16f, 1.0f), 0.03f);
            Params.BiomeColorGrassland  = JitterColor(Stream, FLinearColor(0.34f, 0.38f, 0.28f, 1.0f), 0.04f);
            Params.BiomeColorForest     = JitterColor(Stream, FLinearColor(0.14f, 0.25f, 0.16f, 1.0f), 0.03f);
            Params.BiomeColorDesert     = JitterColor(Stream, FLinearColor(0.64f, 0.68f, 0.72f, 1.0f), 0.04f);
            Params.BiomeColorSavannah   = JitterColor(Stream, FLinearColor(0.48f, 0.50f, 0.44f, 1.0f), 0.04f);
            Params.BiomeColorRainforest = JitterColor(Stream, FLinearColor(0.10f, 0.20f, 0.14f, 1.0f), 0.03f);

            Params.StrataColor1 = JitterColor(Stream, FLinearColor(0.52f, 0.56f, 0.62f, 1.0f), 0.04f);
            Params.StrataColor2 = JitterColor(Stream, FLinearColor(0.34f, 0.38f, 0.44f, 1.0f), 0.04f);
            Params.StrataColor3 = JitterColor(Stream, FLinearColor(0.18f, 0.20f, 0.25f, 1.0f), 0.03f);
            break;
        }

        case ECosmicPlanetArchetype::CrateredBarren:
        {
            const int32 SubPalette = Stream.RandRange(0, 2);
            Params.bEnableSnow = false;
            Params.WetnessIntensity = 0.0f;
            Params.MagmaIntensity = 0.0f;
            Params.SlopeCliffThreshold = 0.09f;
            Params.SlopeCliffFallOff = 0.40f;

            FLinearColor MariaBasalt;
            FLinearColor MidRegolith;
            FLinearColor HighlandDust;
            FLinearColor BrightEjecta;

            if (SubPalette == 0)
            {
                // Mars-Like Iron Oxide & Dark Basalt
                MariaBasalt  = JitterColor(Stream, FLinearColor(0.28f, 0.18f, 0.13f, 1.0f), 0.03f);
                MidRegolith  = JitterColor(Stream, FLinearColor(0.54f, 0.30f, 0.18f, 1.0f), 0.04f);
                HighlandDust = JitterColor(Stream, FLinearColor(0.68f, 0.40f, 0.24f, 1.0f), 0.04f);
                BrightEjecta = JitterColor(Stream, FLinearColor(0.78f, 0.54f, 0.36f, 1.0f), 0.04f);
            }
            else if (SubPalette == 1)
            {
                // Mercury / Lunar Silicate Grey-Bronze
                MariaBasalt  = JitterColor(Stream, FLinearColor(0.22f, 0.21f, 0.20f, 1.0f), 0.03f);
                MidRegolith  = JitterColor(Stream, FLinearColor(0.38f, 0.36f, 0.34f, 1.0f), 0.03f);
                HighlandDust = JitterColor(Stream, FLinearColor(0.54f, 0.52f, 0.49f, 1.0f), 0.04f);
                BrightEjecta = JitterColor(Stream, FLinearColor(0.72f, 0.70f, 0.67f, 1.0f), 0.04f);
            }
            else
            {
                // Callisto / Venus Warm Ochre-Brown Regolith
                MariaBasalt  = JitterColor(Stream, FLinearColor(0.24f, 0.20f, 0.16f, 1.0f), 0.03f);
                MidRegolith  = JitterColor(Stream, FLinearColor(0.45f, 0.36f, 0.26f, 1.0f), 0.04f);
                HighlandDust = JitterColor(Stream, FLinearColor(0.60f, 0.48f, 0.34f, 1.0f), 0.04f);
                BrightEjecta = JitterColor(Stream, FLinearColor(0.74f, 0.62f, 0.46f, 1.0f), 0.04f);
            }

            Params.BiomeColorPolarIce   = BrightEjecta;
            Params.BiomeColorTundra     = MariaBasalt;
            Params.BiomeColorTaiga      = MidRegolith;
            Params.BiomeColorGrassland  = MidRegolith;
            Params.BiomeColorForest     = HighlandDust;
            Params.BiomeColorDesert     = BrightEjecta;
            Params.BiomeColorSavannah   = HighlandDust;
            Params.BiomeColorRainforest = MariaBasalt;

            Params.StrataColor1 = HighlandDust;
            Params.StrataColor2 = MidRegolith;
            Params.StrataColor3 = MariaBasalt;
            break;
        }

        case ECosmicPlanetArchetype::VolcanicScorched:
        {
            Params.bEnableSnow = false;
            Params.WetnessIntensity = 0.0f;
            Params.MagmaIntensity = Stream.FRandRange(0.65f, 1.35f);
            Params.SlopeCliffThreshold = 0.08f;
            Params.SlopeCliffFallOff = 0.35f;

            const FLinearColor Obsidian = JitterColor(Stream, FLinearColor(0.14f, 0.13f, 0.13f, 1.0f), 0.02f);
            const FLinearColor Basalt   = JitterColor(Stream, FLinearColor(0.24f, 0.21f, 0.19f, 1.0f), 0.03f);
            const FLinearColor Scorched = JitterColor(Stream, FLinearColor(0.42f, 0.26f, 0.16f, 1.0f), 0.04f);
            const FLinearColor Sulfur   = JitterColor(Stream, FLinearColor(0.58f, 0.36f, 0.18f, 1.0f), 0.04f);

            Params.BiomeColorPolarIce   = Basalt;
            Params.BiomeColorTundra     = Obsidian;
            Params.BiomeColorTaiga      = Basalt;
            Params.BiomeColorGrassland  = Basalt;
            Params.BiomeColorForest     = Obsidian;
            Params.BiomeColorDesert     = Sulfur;
            Params.BiomeColorSavannah   = Scorched;
            Params.BiomeColorRainforest = Obsidian;

            Params.StrataColor1 = Scorched;
            Params.StrataColor2 = Basalt;
            Params.StrataColor3 = Obsidian;
            break;
        }

        case ECosmicPlanetArchetype::Moon:
        default:
        {
            const int32 SubPalette = Stream.RandRange(0, 3);
            Params.bEnableSnow = false;
            Params.WetnessIntensity = 0.0f;
            Params.MagmaIntensity = 0.0f;
            Params.SlopeCliffThreshold = 0.10f;
            Params.SlopeCliffFallOff = 0.45f;

            FLinearColor Low;
            FLinearColor Mid;
            FLinearColor High;

            if (SubPalette == 0)
            {
                // Classic Lunar Anorthosite & Basalt Maria
                Low  = JitterColor(Stream, FLinearColor(0.22f, 0.23f, 0.24f, 1.0f), 0.02f);
                Mid  = JitterColor(Stream, FLinearColor(0.42f, 0.43f, 0.44f, 1.0f), 0.03f);
                High = JitterColor(Stream, FLinearColor(0.68f, 0.70f, 0.72f, 1.0f), 0.03f);
            }
            else if (SubPalette == 1)
            {
                // Warm Dusty Silicate Moon
                Low  = JitterColor(Stream, FLinearColor(0.25f, 0.22f, 0.19f, 1.0f), 0.02f);
                Mid  = JitterColor(Stream, FLinearColor(0.46f, 0.41f, 0.36f, 1.0f), 0.03f);
                High = JitterColor(Stream, FLinearColor(0.70f, 0.64f, 0.56f, 1.0f), 0.03f);
            }
            else if (SubPalette == 2)
            {
                // Icy Europa / Ganymede Moon
                Low  = JitterColor(Stream, FLinearColor(0.32f, 0.38f, 0.44f, 1.0f), 0.03f);
                Mid  = JitterColor(Stream, FLinearColor(0.56f, 0.64f, 0.72f, 1.0f), 0.03f);
                High = JitterColor(Stream, FLinearColor(0.82f, 0.88f, 0.94f, 1.0f), 0.03f);
            }
            else
            {
                // Iron-Rust Phobos / Callisto Moon
                Low  = JitterColor(Stream, FLinearColor(0.24f, 0.18f, 0.15f, 1.0f), 0.02f);
                Mid  = JitterColor(Stream, FLinearColor(0.44f, 0.33f, 0.26f, 1.0f), 0.03f);
                High = JitterColor(Stream, FLinearColor(0.64f, 0.50f, 0.40f, 1.0f), 0.03f);
            }

            Params.BiomeColorPolarIce   = High;
            Params.BiomeColorTundra     = Low;
            Params.BiomeColorTaiga      = FMath::Lerp(Low, Mid, 0.5f);
            Params.BiomeColorGrassland  = Mid;
            Params.BiomeColorForest     = Mid;
            Params.BiomeColorDesert     = High;
            Params.BiomeColorSavannah   = FMath::Lerp(Mid, High, 0.5f);
            Params.BiomeColorRainforest = Low;

            Params.StrataColor1 = High;
            Params.StrataColor2 = Mid;
            Params.StrataColor3 = Low;
            break;
        }
        }

        return Params;
    }

    static FCosmicPlanetAppearanceParams MakeTelluricAppearance(FRandomStream& Stream)
    {
        return MakeAppearanceForArchetype(ECosmicPlanetArchetype::EarthLike, Stream);
    }

    static FCosmicPlanetAppearanceParams MakeMoonAppearance(FRandomStream& Stream)
    {
        return MakeAppearanceForArchetype(ECosmicPlanetArchetype::Moon, Stream);
    }

    static FCosmicPlanetAppearanceParams MakeStarAppearance(FRandomStream& Stream)
    {
        FCosmicPlanetAppearanceParams Params;
        Params.bEnableSnow = false;
        Params.WetnessIntensity = 0.0f;
        Params.MagmaIntensity = 1.0f;

        const FLinearColor Hot  = FLinearColor(1.0f, 0.85f, 0.45f, 1.0f);
        const FLinearColor Warm = FLinearColor(0.95f, 0.45f, 0.12f, 1.0f);
        const FLinearColor Cool = FLinearColor(0.65f, 0.18f, 0.05f, 1.0f);

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
