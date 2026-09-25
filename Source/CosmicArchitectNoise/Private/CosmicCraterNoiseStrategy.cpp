// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicCraterNoiseStrategy.h"
#include "Math/UnrealMathUtility.h"

void FCosmicCraterNoiseStrategy::Initialize(
    int32 InSeed,
    const FCosmicNoiseLayer& InLayerParameters,
    const FCosmicNoiseCraterParameters& InCraterParameters,
    float InHeightNormalizationScale)
{
    Seed = InSeed;
    LayerParameters = InLayerParameters;
    CraterParameters = InCraterParameters;
    HeightNormalizationScale = InHeightNormalizationScale;

    // 1. Planetary Base Topography (primordial crust & highlands)
    BaseNoise.SetSeed(Seed);
    switch (LayerParameters.NoiseType)
    {
    case ECosmicNoiseType::Perlin:   BaseNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin); break;
    case ECosmicNoiseType::Simplex:  BaseNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2); break;
    case ECosmicNoiseType::Cellular: BaseNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular); break;
    case ECosmicNoiseType::Value:    BaseNoise.SetNoiseType(FastNoiseLite::NoiseType_Value); break;
    case ECosmicNoiseType::Ridged:   BaseNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2); break;
    default:                         BaseNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2); break;
    }

    switch (LayerParameters.FractalType)
    {
    case ECosmicFractalType::None:     BaseNoise.SetFractalType(FastNoiseLite::FractalType_None); break;
    case ECosmicFractalType::FBM:      BaseNoise.SetFractalType(FastNoiseLite::FractalType_FBm); break;
    case ECosmicFractalType::Ridged:   BaseNoise.SetFractalType(FastNoiseLite::FractalType_Ridged); break;
    case ECosmicFractalType::PingPong: BaseNoise.SetFractalType(FastNoiseLite::FractalType_PingPong); break;
    default:                           BaseNoise.SetFractalType(FastNoiseLite::FractalType_FBm); break;
    }

    BaseNoise.SetFrequency(LayerParameters.Frequency);
    BaseNoise.SetFractalOctaves(FMath::Clamp(LayerParameters.Octaves, 1, 8));
    BaseNoise.SetFractalLacunarity(LayerParameters.Lacunarity);
    BaseNoise.SetFractalGain(LayerParameters.Persistence);

    // 2. Cellular Distance Generator for Crater Geometry
    CraterDistNoise.SetSeed(Seed + 512);
    CraterDistNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
    CraterDistNoise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean);
    CraterDistNoise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance);
    CraterDistNoise.SetFrequency(CraterParameters.CraterFrequency);
    CraterDistNoise.SetFractalType(FastNoiseLite::FractalType_None);

    // 3. Cellular Cell-Value Hash for Sparse Crater Distribution (identical seed/frequency)
    CraterCellNoise.SetSeed(Seed + 512);
    CraterCellNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
    CraterCellNoise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean);
    CraterCellNoise.SetCellularReturnType(FastNoiseLite::CellularReturnType_CellValue);
    CraterCellNoise.SetFrequency(CraterParameters.CraterFrequency);
    CraterCellNoise.SetFractalType(FastNoiseLite::FractalType_None);

    // 4. Crater Rim Domain Distortion (breaks circular artificial symmetry)
    CraterDistortNoise.SetSeed(Seed + 777);
    CraterDistortNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    CraterDistortNoise.SetFrequency(CraterParameters.CraterFrequency * 1.5f);

    // 5. Crater Wall & Floor Micro-Breakup (rocky rubble & shattered bedrock)
    CraterBreakupNoise.SetSeed(Seed + 999);
    CraterBreakupNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    CraterBreakupNoise.SetFrequency(CraterParameters.CraterFrequency * 6.0f);

    // Precalculate true elevation bounds across octaves
    float TotalDepth = 0.0f;
    float TotalRim = 0.0f;
    float AmpIter = 1.0f;
    for (int32 i = 0; i < CraterParameters.CraterOctaves; ++i)
    {
        TotalDepth += CraterParameters.CraterDepth * AmpIter * 1.3f;
        TotalRim += CraterParameters.CraterDepth * AmpIter * CraterParameters.CraterRimHeight * 1.3f;
        AmpIter *= CraterParameters.CraterPersistence;
    }

    const float BaseAmp = FMath::Max(1.0f, LayerParameters.Amplitude);
    CachedMinHeight = -TotalDepth - BaseAmp;
    CachedMaxHeight = BaseAmp + TotalRim;
}

void FCosmicCraterNoiseStrategy::Initialize(
    int32 InSeed,
    const FCosmicNoiseLayer& InLayerParameters,
    const FCosmicNoiseBiomeParameters& /*InBiomeParameters*/,
    const FCosmicNoiseCraterParameters& InCraterParameters)
{
    Initialize(InSeed, InLayerParameters, InCraterParameters, 1.0f);
}

void FCosmicCraterNoiseStrategy::EvaluatePoint(
    const FVector& NoiseDir,
    float& OutHeight,
    FLinearColor& OutColor) const
{
    const float X = NoiseDir.X;
    const float Y = NoiseDir.Y;
    const float Z = NoiseDir.Z;

    // 1. Primordial Planetary Crust / Highlands Topography
    const float BaseHeight = BaseNoise.GetNoise(X, Y, Z) * LayerParameters.Amplitude;
    float Height = BaseHeight;

    // 2. Multi-Scale Impact Cratering
    float AccumulatedCraterHeight = 0.0f;
    float AccumulatedEjectaBrightness = 0.0f;

    float FreqScale = 1.0f;
    float AmpScale = 1.0f;

    for (int32 Oct = 0; Oct < CraterParameters.CraterOctaves; ++Oct)
    {
        const float OctOffset = Oct * 133.7f;
        float SampleX = (X * FreqScale) + OctOffset;
        float SampleY = (Y * FreqScale) + OctOffset;
        float SampleZ = (Z * FreqScale) + OctOffset;

        // Rim Distortion (perturbs sampling coordinates so craters aren't perfectly circular)
        if (CraterParameters.CraterDistortion > 0.001f)
        {
            const float DistortFactor = CraterParameters.CraterDistortion * 0.15f;
            SampleX += CraterDistortNoise.GetNoise(SampleX, SampleY, SampleZ) * DistortFactor;
            SampleY += CraterDistortNoise.GetNoise(SampleY + 31.0f, SampleZ, SampleX) * DistortFactor;
            SampleZ += CraterDistortNoise.GetNoise(SampleZ + 67.0f, SampleX, SampleY) * DistortFactor;
        }

        // Cell Hash for Sparse Stochastic Distribution
        // In CellValue mode, FastNoiseLite returns a pseudorandom constant [-1, 1] per Voronoi cell
        const float CellRaw = CraterCellNoise.GetNoise(SampleX, SampleY, SampleZ);
        const float CellHash = (CellRaw + 1.0f) * 0.5f; // [0, 1]

        // Skip cells that don't pass the crater density threshold (eliminates golf-ball honeycomb!)
        if (CellHash > CraterParameters.CraterDensity)
        {
            FreqScale *= CraterParameters.CraterLacunarity;
            AmpScale *= CraterParameters.CraterPersistence;
            continue;
        }

        // Cellular Distance to Nearest Center [0, 1]
        const float RawDist = CraterDistNoise.GetNoise(SampleX, SampleY, SampleZ);
        const float CellDistance = (RawDist + 1.0f) * 0.5f;

        // Power-law size variation derived from CellHash (creates variety: small, medium, and giant craters)
        const float SizeMultiplier = FMath::Lerp(0.6f, 1.4f, CellHash) * CraterParameters.CraterRadiusMultiplier;
        const float DynamicRadius = FMath::Max(0.01f, SizeMultiplier);
        const float CurrentDepth = CraterParameters.CraterDepth * AmpScale * FMath::Lerp(0.7f, 1.3f, CellHash);

        // Crater influence radius covers the cavity, rim, and outer ejecta blanket
        const float InfluenceRadius = DynamicRadius * 2.2f;

        if (CellDistance < InfluenceRadius)
        {
            const float t = CellDistance / DynamicRadius; // 0 = Center, 1.0 = Rim Crest
            float CraterDisplacement = 0.0f;
            const float PeakRimHeight = CurrentDepth * CraterParameters.CraterRimHeight;

            // A. Interior Cavity (t < 1.0)
            if (t < 1.0f)
            {
                const float FloorStart = FMath::Clamp(CraterParameters.CraterFloorHeight, 0.0f, 0.90f);
                float Bowl = 0.0f;

                if (t <= FloorStart)
                {
                    // Flat floor zone (typical of large lunar craters with melted breccia)
                    Bowl = 1.0f;
                }
                else
                {
                    // Hermite C1 continuous curve: Bowl(FloorStart) = 1, Bowl(1.0) = 0, with zero derivative at both ends
                    const float u = (t - FloorStart) / FMath::Max(0.001f, 1.0f - FloorStart);
                    const float S = FMath::SmoothStep(0.0f, 1.0f, u);
                    Bowl = 1.0f - S;
                }

                // Excavate cavity into negative displacement
                CraterDisplacement -= Bowl * CurrentDepth;

                // Central Rebound Peak (Elastic isostatic rebound for complex craters)
                if (CraterParameters.CentralPeakHeight > 0.001f && (Oct == 0 || CurrentDepth > 80.0f))
                {
                    const float PeakRadius = FMath::Clamp(CraterParameters.CentralPeakRadius, 0.05f, 0.45f);
                    if (t < PeakRadius)
                    {
                        const float PeakT = t / PeakRadius;
                        const float PeakProfile = FMath::Pow(FMath::Cos(PeakT * HALF_PI), 2.5f);
                        const float PeakHeight = PeakProfile * CurrentDepth * CraterParameters.CentralPeakHeight;
                        CraterDisplacement += PeakHeight;

                        // Central peaks expose fresh, highly reflective bedrock
                        AccumulatedEjectaBrightness += PeakProfile * 0.5f;
                    }
                }

                // Raised Rim - Inner slope rising up to t = 1.0
                const float InnerRimWidth = 0.22f;
                const float RimDist = (1.0f - t) / InnerRimWidth;
                const float RimExponent = (RimDist * RimDist) * CraterParameters.CraterRimSharpness;
                if (RimExponent < 16.0f)
                {
                    const float Rim = FMath::Exp(-RimExponent) * PeakRimHeight;
                    CraterDisplacement += Rim;
                    AccumulatedEjectaBrightness += FMath::Exp(-RimExponent) * 0.7f;
                }
            }
            else
            {
                // B. Exterior Rim Descent & Ejecta Blanket (t >= 1.0)
                // Seamless C1 transition: at t = 1.0, value is EXACTLY PeakRimHeight and derivative is 0!
                const float OuterDist = t - 1.0f; // 0.0 at rim crest

                // Outer slope width expands with EjectaStrength for a natural apron
                const float OuterRimWidth = 0.22f + CraterParameters.EjectaStrength * 0.25f;
                const float OuterRimRatio = OuterDist / FMath::Max(0.001f, OuterRimWidth);
                const float OuterExponent = (OuterRimRatio * OuterRimRatio) * (CraterParameters.CraterRimSharpness * 0.75f);

                // Smooth asymptotic fade out to zero at InfluenceRadius (eliminates any boundary cut)
                const float OuterFade = 1.0f - FMath::SmoothStep(1.3f, 2.1f, t);

                if (OuterExponent < 18.0f && OuterFade > 0.0001f)
                {
                    const float OuterRim = FMath::Exp(-OuterExponent) * PeakRimHeight * OuterFade;
                    CraterDisplacement += OuterRim;
                    AccumulatedEjectaBrightness += FMath::Exp(-OuterExponent) * 0.7f * OuterFade;
                }
            }

            // C. Wall & Floor Micro-Breakup (Rubble, talus, and fractured rock faces)
            if (CraterParameters.CraterNoiseBreakup > 0.001f && t < 1.3f)
            {
                const float Breakup = CraterBreakupNoise.GetNoise(SampleX * 4.0f, SampleY * 4.0f, SampleZ * 4.0f);
                const float BreakupMask = 1.0f - FMath::SmoothStep(0.85f, 1.25f, t);
                CraterDisplacement += Breakup * (CurrentDepth * 0.05f * CraterParameters.CraterNoiseBreakup) * BreakupMask;
            }

            AccumulatedCraterHeight += CraterDisplacement;
        }

        FreqScale *= CraterParameters.CraterLacunarity;
        AmpScale *= CraterParameters.CraterPersistence;
    }

    Height += AccumulatedCraterHeight;

    // 3. True Elevation Normalization (Prevents clamping crater interiors to 0.0!)
    const float NormScale = FMath::Max(0.01f, HeightNormalizationScale);
    const float NormMin = CachedMinHeight * NormScale;
    const float NormMax = CachedMaxHeight * NormScale;
    const float AltitudeNormalized = FMath::Clamp(
        (Height - NormMin) / FMath::Max(0.001f, NormMax - NormMin),
        0.0f,
        1.0f
    );

    // 4. Lunar / Planetary Color Encoding
    // Channel G: Fresh Ejecta Rays & High Albedo Pulverized Regolith
    const float FreshEjecta = FMath::Clamp(AccumulatedEjectaBrightness, 0.0f, 1.0f);

    // Channel B: Lunar Maria (dark basalt plains in deep basins) vs Highlands (bright anorthosite)
    const float MariaThreshold = FMath::Clamp(CraterParameters.MariaThreshold, 0.05f, 0.95f);
    const float HighlandsVsMaria = FMath::SmoothStep(MariaThreshold - 0.08f, MariaThreshold + 0.08f, AltitudeNormalized);

    // OUTPUT
    OutHeight = Height;
    OutColor = FLinearColor(
        AltitudeNormalized,  // R: True Altitude [0, 1] without negative crushing
        FreshEjecta,         // G: Fresh Ray & Ejecta Blanket Albedo
        HighlandsVsMaria,    // B: 0 = Dark Basaltic Maria, 1 = Bright Anorthosite Highlands
        1.0f                 // A: Valid Alpha
    );
}
