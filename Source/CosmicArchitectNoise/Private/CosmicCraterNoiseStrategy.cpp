// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicCraterNoiseStrategy.h"
#include "Math/UnrealMathUtility.h"

void FCosmicCraterNoiseStrategy::Initialize(const UCosmicCraterNoiseSettings& InSettings)
{
    Seed = InSettings.Seed;
    LayerParameters = InSettings.LayerParameters;
    CraterParameters = InSettings.CraterParameters;
    HeightNormalizationScale = InSettings.HeightNormalizationScale;

    RidgeLayer = InSettings.RidgeLayer;
    DetailLayer = InSettings.DetailLayer;
    TerrainParameters = InSettings.TerrainParameters;
    DomainWarpParams = InSettings.DomainWarpParams;
    BiomeParameters = InSettings.BiomeParameters;

    SetupGenerators();
}

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

    // Populate calibrated defaults for newly added terrain & climate layers
    RidgeLayer = { 8.0f, 6, 2.1f, 0.52f, 0.65f };
    DetailLayer = { 80.0f, 4, 2.0f, 0.45f, 0.12f };
    TerrainParameters = FCosmicCraterTerrainParameters();

    DomainWarpParams.bUseDomainWarp = true;
    DomainWarpParams.DomainWarpStrength = 0.25f;
    DomainWarpParams.DomainWarpFrequency = 1.4f;
    DomainWarpParams.DomainWarpOctaves = 3;

    BiomeParameters.TemperatureFrequency = 3.5f;
    BiomeParameters.LatitudeEffect = 0.55f;
    BiomeParameters.AltitudeTemperaturePenalty = 0.55f;
    BiomeParameters.HumidityFrequency = 12.0f;
    BiomeParameters.HumidityOctaves = 4;
    BiomeParameters.HumidityContrast = 1.45f;
    BiomeParameters.HumidityOffset = 0.0f;

    SetupGenerators();
}

void FCosmicCraterNoiseStrategy::Initialize(
    int32 InSeed,
    const FCosmicNoiseLayer& InLayerParameters,
    const FCosmicNoiseBiomeParameters& InBiomeParameters,
    const FCosmicNoiseCraterParameters& InCraterParameters)
{
    Initialize(InSeed, InLayerParameters, InCraterParameters, 1.0f);
    BiomeParameters = InBiomeParameters;
    SetupGenerators();
}

void FCosmicCraterNoiseStrategy::SetupGenerators()
{
    // 1. Crustal Domain Warping (Inigo Quilez progressive warp for natural geological flow)
    DomainWarpNoise.SetSeed(Seed + 400);
    DomainWarpNoise.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);
    DomainWarpNoise.SetFractalType(FastNoiseLite::FractalType_DomainWarpProgressive);
    DomainWarpNoise.SetFractalOctaves(FMath::Clamp(DomainWarpParams.DomainWarpOctaves, 1, 6));
    DomainWarpNoise.SetFrequency(FMath::Max(0.01f, DomainWarpParams.DomainWarpFrequency));
    DomainWarpNoise.SetDomainWarpAmp(DomainWarpParams.DomainWarpStrength);

    // 2. Continent-Scale Maria Basins vs Primordial Highlands Mask
    MariaBasinNoise.SetSeed(Seed + 50);
    MariaBasinNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    MariaBasinNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    MariaBasinNoise.SetFrequency(FMath::Clamp(LayerParameters.Frequency * 0.30f, 0.6f, 2.0f));
    MariaBasinNoise.SetFractalOctaves(4);
    MariaBasinNoise.SetFractalLacunarity(2.0f);
    MariaBasinNoise.SetFractalGain(0.5f);

    // 3. Planetary Base Topography (primordial crust & rolling highland relief)
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
    BaseNoise.SetFractalOctaves(FMath::Clamp(LayerParameters.Octaves, 1, 12));
    BaseNoise.SetFractalLacunarity(LayerParameters.Lacunarity);
    BaseNoise.SetFractalGain(LayerParameters.Persistence);

    // 4. Hybrid Ridged Multifractal Mountains (Montes) & Wrinkle Ridges (Dorsa)
    RidgeNoise.SetSeed(Seed + 150);
    RidgeNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    RidgeNoise.SetFractalType(FastNoiseLite::FractalType_Ridged);
    RidgeNoise.SetFrequency(FMath::Max(0.01f, RidgeLayer.Frequency));
    RidgeNoise.SetFractalOctaves(FMath::Clamp(RidgeLayer.Octaves, 1, 10));
    RidgeNoise.SetFractalLacunarity(RidgeLayer.Lacunarity);
    RidgeNoise.SetFractalGain(RidgeLayer.Persistence);
    RidgeNoise.SetFractalWeightedStrength(FMath::Clamp(TerrainParameters.RidgeWeightedStrength, 0.0f, 1.0f));

    // 5. High-Frequency Heterogeneous Micro-Detail (ValueCubic for organic rock facets & regolith)
    DetailNoise.SetSeed(Seed + 300);
    DetailNoise.SetNoiseType(FastNoiseLite::NoiseType_ValueCubic);
    DetailNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    DetailNoise.SetFrequency(FMath::Max(0.01f, DetailLayer.Frequency));
    DetailNoise.SetFractalOctaves(FMath::Clamp(DetailLayer.Octaves, 1, 8));
    DetailNoise.SetFractalLacunarity(DetailLayer.Lacunarity);
    DetailNoise.SetFractalGain(DetailLayer.Persistence);

    // 6. Cellular Distance Generator for Crater Geometry
    const float SafeCraterFreq = FMath::Max(0.0001f, CraterParameters.CraterFrequency);
    CraterDistNoise.SetSeed(Seed + 512);
    CraterDistNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
    CraterDistNoise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean);
    CraterDistNoise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance);
    CraterDistNoise.SetFrequency(SafeCraterFreq);
    CraterDistNoise.SetFractalType(FastNoiseLite::FractalType_None);

    // 7. Cellular Cell-Value Hash for Sparse Crater Distribution (identical seed/frequency)
    CraterCellNoise.SetSeed(Seed + 512);
    CraterCellNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
    CraterCellNoise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean);
    CraterCellNoise.SetCellularReturnType(FastNoiseLite::CellularReturnType_CellValue);
    CraterCellNoise.SetFrequency(SafeCraterFreq);
    CraterCellNoise.SetFractalType(FastNoiseLite::FractalType_None);

    // 8. Crater Rim Domain Distortion (breaks circular artificial symmetry)
    CraterDistortNoise.SetSeed(Seed + 777);
    CraterDistortNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    CraterDistortNoise.SetFrequency(SafeCraterFreq * 1.5f);

    // 9. Crater Wall, Floor & Ejecta Micro-Breakup (rocky rubble & irregular ejecta rays)
    CraterBreakupNoise.SetSeed(Seed + 999);
    CraterBreakupNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    CraterBreakupNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    CraterBreakupNoise.SetFractalOctaves(3);
    CraterBreakupNoise.SetFrequency(SafeCraterFreq * 4.5f);

    // 10. Planetary Climate Generators (Temperature & Humidity for full-globe biomes)
    TempNoise.SetSeed(Seed + 1100);
    TempNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    TempNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    TempNoise.SetFrequency(FMath::Max(0.0001f, BiomeParameters.TemperatureFrequency));
    TempNoise.SetFractalOctaves(3);
    TempNoise.SetFractalLacunarity(2.0f);
    TempNoise.SetFractalGain(0.5f);

    HumidityNoise.SetSeed(Seed + 1200);
    HumidityNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    HumidityNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    HumidityNoise.SetFrequency(FMath::Max(0.0001f, BiomeParameters.HumidityFrequency));
    HumidityNoise.SetFractalOctaves(FMath::Clamp(BiomeParameters.HumidityOctaves, 1, 8));
    HumidityNoise.SetFractalLacunarity(2.0f);
    HumidityNoise.SetFractalGain(0.5f);

    // 11. Precalculate realistic statistical elevation bounds for full [0, 1] AltitudeNormalized dynamic range
    const float BaseAmp = FMath::Max(1.0f, LayerParameters.Amplitude);
    const float RidgeAmp = BaseAmp * FMath::Max(0.0f, RidgeLayer.Amplitude);
    const float DetailAmp = BaseAmp * FMath::Max(0.0f, DetailLayer.Amplitude);

    const float SecondaryCraterFactor = (CraterParameters.CraterOctaves > 1)
        ? (1.0f + FMath::Clamp(CraterParameters.CraterPersistence, 0.0f, 0.85f) * 0.45f)
        : 1.0f;
    const float EffectiveCraterDepth = CraterParameters.CraterDepth * SecondaryCraterFactor;
    const float EffectiveRimBoost = CraterParameters.CraterDepth * FMath::Min(CraterParameters.CraterRimHeight, 1.25f) * 0.70f;

    CachedMinHeight = -(BaseAmp * (0.65f + TerrainParameters.MariaBasinDepthScale * 0.45f) + EffectiveCraterDepth * 0.85f);
    CachedMaxHeight = BaseAmp * 0.90f + RidgeAmp * 0.75f + DetailAmp * 0.45f + EffectiveRimBoost;

    if (FMath::IsNearlyEqual(CachedMaxHeight, CachedMinHeight))
    {
        CachedMaxHeight = CachedMinHeight + 1.0f;
    }
}

void FCosmicCraterNoiseStrategy::EvaluatePoint(
    const FVector& NoiseDir,
    float& OutHeight,
    FLinearColor& OutColor) const
{
    const float X = NoiseDir.X;
    const float Y = NoiseDir.Y;
    const float Z = NoiseDir.Z;

    // 1. Crustal Domain Warping (Inigo Quilez organic geological flow)
    float WarpedX = X;
    float WarpedY = Y;
    float WarpedZ = Z;
    if (DomainWarpParams.bUseDomainWarp && DomainWarpParams.DomainWarpStrength > KINDA_SMALL_NUMBER)
    {
        DomainWarpNoise.DomainWarp(WarpedX, WarpedY, WarpedZ);
    }

    // 2. Macro Basaltic Maria Basins vs Primordial Highlands
    const float BaseAmp = FMath::Max(1.0f, LayerParameters.Amplitude);
    const float MariaRaw = MariaBasinNoise.GetNoise(WarpedX, WarpedY, WarpedZ);
    const float MariaVal = (MariaRaw + 1.0f) * 0.5f; // [0, 1]
    const float MariaThreshold = FMath::Clamp(CraterParameters.MariaThreshold, 0.05f, 0.95f);

    // Smooth C1 transition from low basaltic maria (0.0) to rugged highlands (1.0)
    const float HighlandMask = FMath::SmoothStep(MariaThreshold - 0.14f, MariaThreshold + 0.14f, MariaVal);
    const float MariaMask = 1.0f - HighlandMask;

    // Hypsometric macro crust: depressed flat-ish maria floors, elevated highland shields
    const float MariaDepression = -BaseAmp * TerrainParameters.MariaBasinDepthScale * MariaMask * (0.35f + 0.65f * (1.0f - FMath::Clamp(MariaVal / MariaThreshold, 0.0f, 1.0f)));
    const float HighlandPlateau = (MariaVal - MariaThreshold) * BaseAmp * (0.55f + 0.45f * HighlandMask);
    const float MacroCrustHeight = MariaDepression + HighlandPlateau;

    // Medium-scale crustal relief (damped in maria basins for smooth lava plains, rich in highlands)
    const float RawBase = BaseNoise.GetNoise(WarpedX, WarpedY, WarpedZ);
    const float MariaRoughness = FMath::Clamp(TerrainParameters.MariaRoughnessFactor, 0.0f, 1.0f);
    const float BaseRelief = RawBase * BaseAmp * FMath::Lerp(MariaRoughness, 0.72f, HighlandMask);

    // 3. Hybrid Ridged Multifractal Mountain Chains (Montes) & Maria Wrinkle Ridges (Dorsa)
    float RidgeVal = 0.0f;
    float RidgeHeight = 0.0f;
    if (RidgeLayer.Amplitude > KINDA_SMALL_NUMBER)
    {
        const float RawRidge = RidgeNoise.GetNoise(WarpedX, WarpedY, WarpedZ);
        RidgeVal = FMath::Max(0.0f, RawRidge);
        if (TerrainParameters.RidgePeakSharpness > 1.0f)
        {
            RidgeVal = FMath::Pow(RidgeVal, TerrainParameters.RidgePeakSharpness);
        }
        // Prominent mountain chains in highlands; subtle tectonic wrinkle ridges (dorsa) across maria
        const float RidgeZoneWeight = FMath::Lerp(0.14f, 1.0f, HighlandMask);
        RidgeHeight = RidgeVal * (BaseAmp * RidgeLayer.Amplitude) * RidgeZoneWeight;
    }

    // 4. Musgrave Heterogeneous Micro-Detail (smooth dusty maria vs fractured highland bedrock)
    float DetailHeight = 0.0f;
    if (DetailLayer.Amplitude > KINDA_SMALL_NUMBER)
    {
        const float HeteroRoughness = FMath::Clamp(
            FMath::Lerp(MariaRoughness, 1.0f, FMath::SmoothStep(0.05f, 0.85f, HighlandMask * 0.75f + RidgeVal * 0.6f)),
            0.05f,
            1.0f
        );
        DetailHeight = DetailNoise.GetNoise(WarpedX, WarpedY, WarpedZ) * (BaseAmp * DetailLayer.Amplitude) * HeteroRoughness;
    }

    float TerrainHeight = MacroCrustHeight + BaseRelief + RidgeHeight + DetailHeight;

    // 5. Multi-Scale Impact Cratering (Sparse Voronoi with Age Weathering, Maria Flooding & Floor Excavation)
    float AccumulatedCraterHeight = 0.0f;
    float AccumulatedEjecta = 0.0f;
    float AccumulatedCraterFloor = 0.0f;
    float FloorFlattenMask = 0.0f;

    float FreqScale = 1.0f;
    float AmpScale = 1.0f;
    const float SafeCraterFreq = FMath::Max(0.25f, CraterParameters.CraterFrequency);
    const float SafeDensity = FMath::Max(0.01f, CraterParameters.CraterDensity);

    for (int32 Oct = 0; Oct < CraterParameters.CraterOctaves; ++Oct)
    {
        const float OctOffset = Oct * 133.7f;
        float SampleX = (X * FreqScale) + OctOffset;
        float SampleY = (Y * FreqScale) + OctOffset;
        float SampleZ = (Z * FreqScale) + OctOffset;

        // Frequency-independent rim distortion (consistent fraction of a Voronoi cell across all frequencies)
        if (CraterParameters.CraterDistortion > 0.001f)
        {
            const float DistortFactor = (CraterParameters.CraterDistortion * 0.16f) / SafeCraterFreq;
            SampleX += CraterDistortNoise.GetNoise(SampleX, SampleY, SampleZ) * DistortFactor;
            SampleY += CraterDistortNoise.GetNoise(SampleY + 31.0f, SampleZ, SampleX) * DistortFactor;
            SampleZ += CraterDistortNoise.GetNoise(SampleZ + 67.0f, SampleX, SampleY) * DistortFactor;
        }

        // Cell Hash for Sparse Stochastic Distribution
        const float CellRaw = CraterCellNoise.GetNoise(SampleX, SampleY, SampleZ);
        const float CellHash = (CellRaw + 1.0f) * 0.5f; // [0, 1]

        if (CellHash > CraterParameters.CraterDensity)
        {
            FreqScale *= CraterParameters.CraterLacunarity;
            AmpScale *= CraterParameters.CraterPersistence;
            continue;
        }

        // Re-normalize hash across active craters [0, 1] so full size/depth variety is preserved at any density
        const float NormHash = FMath::Clamp(CellHash / SafeDensity, 0.0f, 1.0f);
        // Independent pseudo-random crater age [0 = fresh crisp impact, 1 = ancient eroded crater]
        const float AgeHash = FMath::Frac(NormHash * 17.31f + static_cast<float>(Oct) * 0.37f);
        const float Weathering = AgeHash * TerrainParameters.CraterAgeVariation;

        // Deep basaltic maria flood and soften older impact craters
        const float MariaFloodAtten = 1.0f - (MariaMask * TerrainParameters.MariaCraterFlooding * (0.30f + 0.70f * AgeHash));

        // Cellular Distance to Nearest Center
        const float RawDist = CraterDistNoise.GetNoise(SampleX, SampleY, SampleZ);
        const float CellDistance = (RawDist + 1.0f) * 0.5f;

        // Power-law size variation clamped safely below Voronoi cell half-width to avoid polygonal clipping
        const float SizeMultiplier = FMath::Lerp(0.55f, 1.35f, NormHash) * CraterParameters.CraterRadiusMultiplier;
        const float DynamicRadius = FMath::Clamp(SizeMultiplier, 0.01f, 0.42f);
        const float CurrentDepth = CraterParameters.CraterDepth * AmpScale
            * FMath::Lerp(0.65f, 1.35f, NormHash)
            * FMath::Lerp(1.0f, 0.60f, Weathering)
            * MariaFloodAtten;

        // Influence radius covers cavity, raised rim, and outer ejecta apron
        const float InfluenceRadius = DynamicRadius * 1.95f;

        if (CellDistance < InfluenceRadius)
        {
            const float t = CellDistance / DynamicRadius; // 0 = Center, 1.0 = Rim Crest
            float CraterDisplacement = 0.0f;

            // Older craters have softer, lower rims
            const float EffectiveRimHeight = CraterParameters.CraterRimHeight * FMath::Lerp(1.0f, 0.55f, Weathering);
            const float EffectiveRimSharpness = CraterParameters.CraterRimSharpness * FMath::Lerp(1.0f, 0.55f, Weathering);
            const float PeakRimHeight = CurrentDepth * EffectiveRimHeight;
            const float FreshnessFactor = 1.0f - Weathering;

            // A. Interior Cavity (t < 1.0)
            if (t < 1.0f)
            {
                const float FloorStart = FMath::Clamp(CraterParameters.CraterFloorHeight, 0.0f, 0.88f);
                float Bowl = 0.0f;

                if (t <= FloorStart)
                {
                    Bowl = 1.0f;
                }
                else
                {
                    const float u = (t - FloorStart) / FMath::Max(0.001f, 1.0f - FloorStart);
                    float ShapedU = u;

                    // Subtle slumped inner-wall terraces on large complex craters (C-infinity smooth)
                    if (Oct <= 1 && TerrainParameters.InnerWallTerracing > 0.01f)
                    {
                        const float WallMask = FMath::Sin(u * PI);
                        ShapedU = FMath::Clamp(
                            u - (0.055f * TerrainParameters.InnerWallTerracing * WallMask) * FMath::Sin(u * 4.0f * PI),
                            0.0f,
                            1.0f
                        );
                    }

                    const float S = FMath::SmoothStep(0.0f, 1.0f, ShapedU);
                    Bowl = 1.0f - S;
                }

                // Excavate cavity
                CraterDisplacement -= Bowl * CurrentDepth;

                // Track floor flattening mask so pre-existing mountains don't poke through melted crater floors
                const float OctFlattenWeight = (Oct == 0) ? 1.0f : ((Oct == 1) ? 0.55f : 0.20f);
                FloorFlattenMask = FMath::Max(FloorFlattenMask, Bowl * OctFlattenWeight * MariaFloodAtten);
                AccumulatedCraterFloor = FMath::Max(AccumulatedCraterFloor, Bowl * (Oct <= 1 ? 1.0f : 0.45f));

                // Central Rebound Peak (gated to larger complex craters only)
                if (CraterParameters.CentralPeakHeight > 0.001f && Oct <= 1 && NormHash > 0.35f)
                {
                    const float PeakRadius = FMath::Clamp(CraterParameters.CentralPeakRadius, 0.05f, 0.45f);
                    if (t < PeakRadius)
                    {
                        const float PeakT = t / PeakRadius;
                        const float PeakProfile = FMath::Pow(FMath::Cos(PeakT * HALF_PI), 2.5f);
                        const float PeakHeight = PeakProfile * CurrentDepth * CraterParameters.CentralPeakHeight;
                        CraterDisplacement += PeakHeight;
                        AccumulatedEjecta += PeakProfile * 0.45f * FreshnessFactor;
                    }
                }

                // Raised Rim - Inner slope rising smoothly to t = 1.0
                const float InnerRimWidth = 0.22f;
                const float RimDist = (1.0f - t) / InnerRimWidth;
                const float RimExponent = (RimDist * RimDist) * EffectiveRimSharpness;
                if (RimExponent < 16.0f)
                {
                    const float RimFactor = FMath::Exp(-RimExponent);
                    CraterDisplacement += RimFactor * PeakRimHeight;
                    AccumulatedEjecta += RimFactor * 0.60f * FreshnessFactor;
                }
            }
            else
            {
                // B. Exterior Rim Descent & Broken Ejecta Blanket (t >= 1.0)
                const float OuterDist = t - 1.0f;
                const float OuterRimWidth = 0.20f + CraterParameters.EjectaStrength * 0.22f;
                const float OuterRimRatio = OuterDist / FMath::Max(0.001f, OuterRimWidth);
                const float OuterExponent = (OuterRimRatio * OuterRimRatio) * (EffectiveRimSharpness * 0.75f);

                // Smooth fade to zero before Voronoi cell edge
                const float OuterFade = 1.0f - FMath::SmoothStep(1.25f, 1.92f, t);

                if (OuterExponent < 18.0f && OuterFade > 0.0001f)
                {
                    // Break up outer ejecta apron organically so rims don't look like uniform stamped rings
                    float EjectaMod = 1.0f;
                    if (TerrainParameters.EjectaRayBreakup > 0.01f && OuterDist > 0.02f)
                    {
                        const float RayNoise = (CraterBreakupNoise.GetNoise(SampleX * 2.5f, SampleY * 2.5f, SampleZ * 2.5f) + 1.0f) * 0.5f;
                        const float OuterBlend = FMath::SmoothStep(0.02f, 0.35f, OuterDist);
                        EjectaMod = FMath::Lerp(1.0f, FMath::Lerp(1.0f - TerrainParameters.EjectaRayBreakup * 0.75f, 1.15f, RayNoise), OuterBlend);
                    }

                    const float RimFactor = FMath::Exp(-OuterExponent) * OuterFade;
                    CraterDisplacement += RimFactor * PeakRimHeight * EjectaMod;
                    AccumulatedEjecta += RimFactor * 0.55f * EjectaMod * FreshnessFactor;
                }
            }

            // C. Wall & Floor Micro-Breakup (talus rubble and fractured rock faces)
            if (CraterParameters.CraterNoiseBreakup > 0.001f && t < 1.3f)
            {
                const float Breakup = CraterBreakupNoise.GetNoise(SampleX * 3.5f, SampleY * 3.5f, SampleZ * 3.5f);
                const float BreakupMask = 1.0f - FMath::SmoothStep(0.85f, 1.25f, t);
                CraterDisplacement += Breakup * (CurrentDepth * 0.045f * CraterParameters.CraterNoiseBreakup) * BreakupMask;
            }

            AccumulatedCraterHeight += CraterDisplacement;
        }

        FreqScale *= CraterParameters.CraterLacunarity;
        AmpScale *= CraterParameters.CraterPersistence;
    }

    // Dampen pre-existing mountainous relief inside excavated crater floors for realistic melted breccia flats
    if (FloorFlattenMask > 0.001f && TerrainParameters.CraterFloorFlattening > 0.001f)
    {
        const float FlattenAlpha = FMath::Clamp(FloorFlattenMask * TerrainParameters.CraterFloorFlattening, 0.0f, 0.90f);
        TerrainHeight = FMath::Lerp(TerrainHeight, MacroCrustHeight + DetailHeight * 0.35f, FlattenAlpha);
    }

    const float FinalHeight = TerrainHeight + AccumulatedCraterHeight;

    // 6. True Elevation Normalization across realistic planetary dynamic range
    const float NormScale = FMath::Max(0.01f, HeightNormalizationScale);
    const float NormMin = CachedMinHeight * NormScale;
    const float NormMax = CachedMaxHeight * NormScale;
    const float AltitudeNormalized = FMath::Clamp(
        (FinalHeight - NormMin) / FMath::Max(0.001f, NormMax - NormMin),
        0.0f,
        1.0f
    );

    // 7. Planetary Temperature (Latitudinal Polar Gradient + Regional Noise - Altitude Lapse Rate)
    // Matches the M_CosmicPlanetaryMaster Whittaker biome contract so biomes span the entire planet!
    const float Latitude = FMath::Abs(Z); // 0 at equator, 1 at poles
    const float BaseTemp = 1.0f - (Latitude * BiomeParameters.LatitudeEffect);
    const float TempVar = TempNoise.GetNoise(WarpedX, WarpedY, WarpedZ) * 0.22f;
    const float SurfaceTemp = FMath::Clamp(BaseTemp + TempVar, 0.0f, 1.0f);

    const float ClampedEjecta = FMath::Clamp(AccumulatedEjecta, 0.0f, 1.0f);
    const float ClampedFloor = FMath::Clamp(AccumulatedCraterFloor, 0.0f, 1.0f);

    // Subtle geological temperature signature from fresh ejecta (bounded so crater rims never hijack biomes)
    const float EjectaTempNudge = ClampedEjecta * TerrainParameters.CraterBiomeInfluence * 0.25f;
    const float VisualTemp = FMath::Clamp(
        SurfaceTemp - (AltitudeNormalized * BiomeParameters.AltitudeTemperaturePenalty) + EjectaTempNudge,
        0.0f,
        1.0f
    );

    // 8. Planetary Humidity / Regolith-Biome Distribution (Full-Globe Regional Noise + Basin Convergence)
    const float RawHum = HumidityNoise.GetNoise(WarpedX, WarpedY, WarpedZ);
    float BaseHumidity = (RawHum + 1.0f) * 0.5f;
    BaseHumidity = FMath::Clamp(
        (BaseHumidity + BiomeParameters.HumidityOffset - 0.5f) * BiomeParameters.HumidityContrast + 0.5f,
        0.0f,
        1.0f
    );

    // Basaltic maria & deep crater floors retain slightly higher basin convergence;
    // exposed fresh ejecta & highland ridges are slightly drier bare rock.
    const float BasinBonus = (MariaMask * 0.16f + ClampedFloor * 0.12f) * (0.5f + TerrainParameters.CraterBiomeInfluence);
    const float EjectaDrying = ClampedEjecta * TerrainParameters.CraterBiomeInfluence * 0.30f;
    const float Humidity = FMath::Clamp(BaseHumidity + BasinBonus - EjectaDrying, 0.0f, 1.0f);

    // 9. Final Output (Standard Planetary Vertex Color Contract)
    OutHeight = FinalHeight;
    OutColor = FLinearColor(
        AltitudeNormalized,  // R: Normalized Altitude [0, 1]
        VisualTemp,          // G: Planetary Visual Temperature [0, 1]
        Humidity,            // B: Planetary Humidity / Biome Distribution [0, 1]
        1.0f                 // A: Valid Alpha
    );
}
