// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicMultiNoiseStrategy.h"

void FCosmicMultiNoiseStrategy::Initialize(
    int32 InSeed,
    float InHeightNormalizationScale,
    const FCosmicNoiseDataLayer& InContinental,
    const FCosmicNoiseDataLayer& InErosion,
    const FCosmicNoiseDataLayer& InPeaksValleys,
    const FCosmicNoiseDataLayer& InDetail,
    const FCosmicMultiNoiseParameters& InMultiNoiseParams,
    const FCosmicNoiseDomainWarpParameters& InDomainWarpParams,
    const FCosmicNoiseBiomeParameters& InBiomeParams,
    const FCosmicOrographicParameters& InOrographicParams)
{
    Seed = InSeed;
    HeightNormalizationScale = InHeightNormalizationScale;

    ContinentalLayer = InContinental;
    ErosionLayer = InErosion;
    PeaksValleysLayer = InPeaksValleys;
    DetailLayer = InDetail;

    MultiNoiseParams = InMultiNoiseParams;
    DomainWarpParams = InDomainWarpParams;
    BiomeParameters = InBiomeParams;
    OrographicParams = InOrographicParams;

    auto SetupSimplexFBM = [&](FastNoiseLite& Noise, const FCosmicNoiseDataLayer& Layer, int32 Offset)
    {
        Noise.SetSeed(Seed + Offset);
        Noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        Noise.SetFractalType(FastNoiseLite::FractalType_FBm);
        Noise.SetFrequency(Layer.Frequency);
        Noise.SetFractalOctaves(Layer.Octaves);
        Noise.SetFractalLacunarity(Layer.Lacunarity);
        Noise.SetFractalGain(Layer.Persistence);
    };

    // 1. Continentalness (Large-scale crustal tectonics)
    SetupSimplexFBM(ContinentalNoise, ContinentalLayer, 0);

    // 2. Erosion (Geological age & relief dampener)
    SetupSimplexFBM(ErosionNoise, ErosionLayer, 200);

    // 3. Peaks & Valleys (Alpine ridged fractal)
    PeaksValleysNoise.SetSeed(Seed + 100);
    PeaksValleysNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    PeaksValleysNoise.SetFractalType(FastNoiseLite::FractalType_Ridged);
    PeaksValleysNoise.SetFrequency(PeaksValleysLayer.Frequency);
    PeaksValleysNoise.SetFractalOctaves(PeaksValleysLayer.Octaves);
    PeaksValleysNoise.SetFractalLacunarity(PeaksValleysLayer.Lacunarity);
    PeaksValleysNoise.SetFractalGain(PeaksValleysLayer.Persistence);

    // 4. Micro Detail (High-frequency ValueCubic)
    DetailNoise.SetSeed(Seed + 300);
    DetailNoise.SetNoiseType(FastNoiseLite::NoiseType_ValueCubic);
    DetailNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    DetailNoise.SetFrequency(DetailLayer.Frequency);
    DetailNoise.SetFractalOctaves(DetailLayer.Octaves);
    DetailNoise.SetFractalLacunarity(DetailLayer.Lacunarity);
    DetailNoise.SetFractalGain(DetailLayer.Persistence);

    // 5. Domain Warping (Inigo Quilez coordinate perturbation)
    DomainWarpNoise.SetSeed(Seed + 400);
    DomainWarpNoise.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);
    DomainWarpNoise.SetFractalType(FastNoiseLite::FractalType_DomainWarpProgressive);
    DomainWarpNoise.SetFractalOctaves(DomainWarpParams.DomainWarpOctaves);
    DomainWarpNoise.SetFrequency(DomainWarpParams.DomainWarpFrequency);
    DomainWarpNoise.SetDomainWarpAmp(DomainWarpParams.DomainWarpStrength);

    // 6. Climate (Humidity & Temperature)
    SetupSimplexFBM(HumidityNoise, { BiomeParameters.HumidityFrequency, BiomeParameters.HumidityOctaves, 2.0f, 0.5f, 1.0f }, 500);
    SetupSimplexFBM(TempNoise, { BiomeParameters.TemperatureFrequency, 3, 2.0f, 0.5f, 1.0f }, 600);
}

FVector FCosmicMultiNoiseStrategy::CalculateWindTangent(const FVector& PointOnSphere) const
{
    if (OrographicParams.bUsePlanetaryZonalWinds)
    {
        const float Z = PointOnSphere.Z;

        // Tangent vector along parallels (Eastward): cross(UpVector, PointOnSphere)
        FVector EastVector = FVector::CrossProduct(FVector::UpVector, PointOnSphere);
        if (EastVector.IsNearlyZero(1e-4f))
        {
            EastVector = FVector(1.0f, 0.0f, 0.0f);
        }
        else
        {
            EastVector.Normalize();
        }

        // Real planetary circulation oscillation:
        // Tropical Hadley cell (|Z| < 0.35): Trade winds blow Westward (-EastVector)
        // Temperate Ferrel cell (0.35 <= |Z| < 0.70): Westerlies blow Eastward (+EastVector)
        // Polar cell (|Z| >= 0.70): Polar easterlies blow Westward (-EastVector)
        const float CirculationOscillation = -FMath::Cos(3.0f * PI * FMath::Abs(Z));
        FVector WindDir = EastVector * CirculationOscillation;

        // Slight equatorward drift in Hadley/Polar cells for realistic trade wind tilt
        FVector NorthVector = FVector::CrossProduct(PointOnSphere, EastVector).GetSafeNormal();
        float EquatorwardDrift = -FMath::Sign(Z) * 0.25f * (1.0f - FMath::Abs(CirculationOscillation));
        WindDir += NorthVector * EquatorwardDrift;

        return WindDir.GetSafeNormal();
    }
    else
    {
        // Custom user prevailing wind projected onto sphere tangent plane
        FVector Tangent = OrographicParams.PrevailingWindDirection - (PointOnSphere * FVector::DotProduct(OrographicParams.PrevailingWindDirection, PointOnSphere));
        if (Tangent.IsNearlyZero(1e-4f))
        {
            return FVector::CrossProduct(PointOnSphere, FVector::UpVector).GetSafeNormal();
        }
        return Tangent.GetSafeNormal();
    }
}

float FCosmicMultiNoiseStrategy::EvaluateHeightOnly(const FVector& NoiseDir) const
{
    float X = NoiseDir.X;
    float Y = NoiseDir.Y;
    float Z = NoiseDir.Z;

    // Domain Warping (Inigo Quilez)
    if (DomainWarpParams.bUseDomainWarp)
    {
        DomainWarpNoise.DomainWarp(X, Y, Z);
    }

    // Continentalness (C)
    const float RawCont = ContinentalNoise.GetNoise(X, Y, Z);
    const float Continentalness = (RawCont + 1.0f) * 0.5f; // [0, 1]

    const float SeaLevel = MultiNoiseParams.SeaLevelThreshold;
    const float ShelfWidth = MultiNoiseParams.ContinentalShelfWidth;
    const float ContAmp = ContinentalLayer.Amplitude;

    float BaseHeight = 0.0f;
    if (Continentalness < SeaLevel)
    {
        // Ocean Basin: smooth transition from abyssal trench to continental slope
        const float ShelfStart = FMath::Max(0.0f, SeaLevel - 0.25f);
        const float OceanT = FMath::Clamp((Continentalness - ShelfStart) / FMath::Max(0.001f, SeaLevel - ShelfStart), 0.0f, 1.0f);
        const float SlopeProfile = FMath::SmoothStep(0.0f, 1.0f, OceanT);
        BaseHeight = FMath::Lerp(-ContAmp * MultiNoiseParams.OceanDepthScale, 0.0f, SlopeProfile);
    }
    else
    {
        // Dry Land: coastal lowlands -> inland plains & tectonic plateaus
        const float LandT = Continentalness - SeaLevel;
        if (LandT < ShelfWidth)
        {
            // Coastal plain & beach transition
            const float CoastFrac = LandT / FMath::Max(0.001f, ShelfWidth);
            BaseHeight = CoastFrac * 0.12f * ContAmp;
        }
        else
        {
            // Continental interior & elevated shields
            const float InlandFrac = (LandT - ShelfWidth) / FMath::Max(0.001f, 1.0f - SeaLevel - ShelfWidth);
            BaseHeight = (0.12f * ContAmp) + (InlandFrac * ContAmp * (0.88f + MultiNoiseParams.InlandPlateauBoost));
        }
    }

    // Erosion (E)
    const float RawErosion = ErosionNoise.GetNoise(X, Y, Z);
    const float Erosion = (RawErosion + 1.0f) * 0.5f; // [0, 1]
    const float ErosionShaped = FMath::Clamp((Erosion - 0.5f) * MultiNoiseParams.ErosionContrast + 0.5f, 0.0f, 1.0f);

    // High erosion flattens relief, low erosion creates sharp mountainous ruggedness
    const float ReliefFactor = FMath::Lerp(1.0f, MultiNoiseParams.HighErosionFlattening, FMath::SmoothStep(0.2f, 0.8f, ErosionShaped));

    // Peaks & Valleys (PV)
    const float RawPV = PeaksValleysNoise.GetNoise(X, Y, Z);
    const float EffectiveSharpness = FMath::Lerp(MultiNoiseParams.MountainPeakSharpness, 1.0f, ErosionShaped);

    float ReliefHeight = 0.0f;
    if (RawPV >= 0.0f)
    {
        // Sharp alpine ridges & mountain horns
        const float PeakNorm = FMath::Pow(RawPV, EffectiveSharpness);
        ReliefHeight = PeakNorm * PeaksValleysLayer.Amplitude;
    }
    else
    {
        // Carved glacial U-valleys and gorges
        const float ValleyNorm = FMath::Pow(FMath::Abs(RawPV), 1.25f);
        ReliefHeight = -ValleyNorm * PeaksValleysLayer.Amplitude * MultiNoiseParams.ValleyDepthMultiplier;
    }

    ReliefHeight *= ReliefFactor;

    // Geological Strata / Terracing
    if (MultiNoiseParams.TerracingStrength > 0.0f && MultiNoiseParams.TerraceSteps > 0.0f)
    {
        const float StepSize = 1000.0f / MultiNoiseParams.TerraceSteps;
        float Terraced = FMath::FloorToFloat(ReliefHeight / StepSize) * StepSize;
        const float StepFrac = (ReliefHeight - Terraced) / StepSize;
        const float SmoothFrac = FMath::SmoothStep(0.2f, 0.8f, StepFrac);
        Terraced += SmoothFrac * StepSize;
        ReliefHeight = FMath::Lerp(ReliefHeight, Terraced, MultiNoiseParams.TerracingStrength * ErosionShaped);
    }

    // Micro Detail
    const float Detail = DetailNoise.GetNoise(X, Y, Z) * DetailLayer.Amplitude;

    // Composition
    const float OceanMask = FMath::SmoothStep(SeaLevel - 0.02f, SeaLevel + 0.02f, Continentalness);
    const float LandHeight = BaseHeight + ReliefHeight + Detail;
    const float OceanHeight = BaseHeight + (Detail * 0.15f);

    return FMath::Lerp(OceanHeight, LandHeight, OceanMask);
}

void FCosmicMultiNoiseStrategy::EvaluatePoint(
    const FVector& NoiseDir,
    float& OutHeight,
    FLinearColor& OutColor) const
{
    // 1. ELEVATION EVALUATION
    const float Height = EvaluateHeightOnly(NoiseDir);

    // 2. ALTITUDE NORMALIZATION
    const float TrueMinHeight = -ContinentalLayer.Amplitude * MultiNoiseParams.OceanDepthScale - DetailLayer.Amplitude;
    const float TrueMaxHeight = ContinentalLayer.Amplitude * (1.0f + MultiNoiseParams.InlandPlateauBoost) + PeaksValleysLayer.Amplitude + DetailLayer.Amplitude;

    const float NormalizedMin = TrueMinHeight * HeightNormalizationScale;
    const float NormalizedMax = TrueMaxHeight * HeightNormalizationScale;
    const float AltitudeNormalized = FMath::Clamp((Height - NormalizedMin) / FMath::Max(0.001f, NormalizedMax - NormalizedMin), 0.0f, 1.0f);

    // 3. TEMPERATURE & LATITUDINAL GRADIENT
    const float Latitude = FMath::Abs(NoiseDir.Z);
    const float BaseTemp = 1.0f - (Latitude * BiomeParameters.LatitudeEffect);
    const float TempVar = TempNoise.GetNoise(NoiseDir.X, NoiseDir.Y, NoiseDir.Z) * 0.25f;
    const float Temperature = FMath::Clamp(BaseTemp + TempVar, 0.0f, 1.0f);

    // 4. ADIABATIC LAPSE RATE (Altitudinal cooling)
    const float VisualTemp = FMath::Clamp(Temperature - (AltitudeNormalized * BiomeParameters.AltitudeTemperaturePenalty), 0.0f, 1.0f);

    // 5. BASE HUMIDITY & OCEAN PROXIMITY
    const float RawCont = ContinentalNoise.GetNoise(NoiseDir.X, NoiseDir.Y, NoiseDir.Z);
    const float Continentalness = (RawCont + 1.0f) * 0.5f;
    const float DistanceFromCoast = FMath::Clamp((Continentalness - MultiNoiseParams.SeaLevelThreshold) / 0.35f, 0.0f, 1.0f);
    const float OceanicMoisture = FMath::Lerp(0.85f, 0.35f, DistanceFromCoast);

    const float RawHum = (HumidityNoise.GetNoise(NoiseDir.X, NoiseDir.Y, NoiseDir.Z) + 1.0f) * 0.5f;
    float Humidity = FMath::Lerp(OceanicMoisture, RawHum, 0.5f);

    // 6. OROGRAPHIC PRECIPITATION & RAIN SHADOW
    if (OrographicParams.bEnableOrographicEffect)
    {
        const FVector WindDir = CalculateWindTangent(NoiseDir);

        // A. Windward slope (Orographic lift: moisture condenses as air rises)
        const FVector UpwindPoint = (NoiseDir - WindDir * OrographicParams.WindwardSampleOffset).GetSafeNormal();
        const float UpwindHeight = EvaluateHeightOnly(UpwindPoint);
        const float SlopeDelta = Height - UpwindHeight;

        if (SlopeDelta > 0.0f && PeaksValleysLayer.Amplitude > 0.0f)
        {
            const float LiftRatio = FMath::Clamp(SlopeDelta / (PeaksValleysLayer.Amplitude * 0.05f + 1.0f), 0.0f, 1.0f);
            Humidity += LiftRatio * OrographicParams.OrographicLiftStrength * 0.35f;
        }

        // B. Leeward rain shadow (Upwind mountain barriers block moisture)
        const FVector BarrierPoint = (NoiseDir - WindDir * OrographicParams.RainShadowDistance).GetSafeNormal();
        const float BarrierHeight = EvaluateHeightOnly(BarrierPoint);

        if (BarrierHeight > Height && BarrierHeight > (PeaksValleysLayer.Amplitude * 0.25f))
        {
            const float BarrierExcess = (BarrierHeight - Height) / (PeaksValleysLayer.Amplitude * 0.6f + 1.0f);
            const float RainShadowDrying = FMath::Clamp(BarrierExcess, 0.0f, 1.0f) * OrographicParams.RainShadowStrength * 0.45f;
            Humidity -= RainShadowDrying;
        }
    }

    // 7. HUMIDITY POST-PROCESSING
    Humidity = (Humidity + BiomeParameters.HumidityOffset);
    Humidity = FMath::Clamp((Humidity - 0.5f) * BiomeParameters.HumidityContrast + 0.5f, 0.0f, 1.0f);

    // 8. STRICT FINAL OUTPUT ASSIGNMENT
    OutHeight = Height;

    OutColor = FLinearColor(
        AltitudeNormalized,  // R
        VisualTemp,          // G
        Humidity,            // B     
        1.0f                 // A 
    );
}
