// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicErosionMultifractalNoiseStrategy.h"
#include "Math/UnrealMathUtility.h"

void FCosmicErosionMultifractalNoiseStrategy::Initialize(const UCosmicErosionMultifractalNoiseSettings& InSettings)
{
	Seed = InSettings.Seed;
	HeightNormalizationScale = InSettings.HeightNormalizationScale;

	ContinentalLayer = InSettings.ContinentalLayer;
	MountainLayer = InSettings.MountainLayer;
	HillLayer = InSettings.HillLayer;
	DetailLayer = InSettings.DetailLayer;
	RiverLayer = InSettings.RiverLayer;

	StrataParameters = InSettings.StrataParameters;
	ErosionParameters = InSettings.ErosionParameters;
	MultifractalParameters = InSettings.MultifractalParameters;
	BiomeParameters = InSettings.BiomeParameters;

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

	// 1. Continental Plates & Oceanic Basins
	SetupSimplexFBM(ContinentalNoise, ContinentalLayer, 0);

	// 2. Mountains (Hybrid Ridged Multifractal with Musgrave Weighted Strength)
	MountainNoise.SetSeed(Seed + 100);
	MountainNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	MountainNoise.SetFractalType(FastNoiseLite::FractalType_Ridged);
	MountainNoise.SetFrequency(MountainLayer.Frequency);
	MountainNoise.SetFractalOctaves(MountainLayer.Octaves);
	MountainNoise.SetFractalLacunarity(MountainLayer.Lacunarity);
	MountainNoise.SetFractalGain(MountainLayer.Persistence);
	MountainNoise.SetFractalWeightedStrength(MultifractalParameters.FractalWeightedStrength);

	// 3. Hills & Sedimentary Basins
	SetupSimplexFBM(HillNoise, HillLayer, 200);

	// 4. Detail Layer (ValueCubic for organic geological micro-facets)
	DetailNoise.SetSeed(Seed + 300);
	DetailNoise.SetNoiseType(FastNoiseLite::NoiseType_ValueCubic);
	DetailNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
	DetailNoise.SetFrequency(DetailLayer.Frequency);
	DetailNoise.SetFractalOctaves(DetailLayer.Octaves);
	DetailNoise.SetFractalLacunarity(DetailLayer.Lacunarity);
	DetailNoise.SetFractalGain(DetailLayer.Persistence);

	// 5. River & Fluvial Drainage Network (Cellular Voronoi Distance2Sub)
	RiverNoise.SetSeed(Seed + 400);
	RiverNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
	RiverNoise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean);
	RiverNoise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2Sub);
	RiverNoise.SetCellularJitter(ErosionParameters.DrainageJitter);
	RiverNoise.SetFrequency(RiverLayer.Frequency);

	// 6. Tectonic Domain Warping
	TectonicWarpNoise.SetSeed(Seed + 500);
	TectonicWarpNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	TectonicWarpNoise.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);
	TectonicWarpNoise.SetFrequency(MultifractalParameters.TectonicWarpFrequency);
	TectonicWarpNoise.SetDomainWarpAmp(MultifractalParameters.TectonicWarpAmplitude);

	// 7. Geological Strata Folding Warping
	StrataWarpNoise.SetSeed(Seed + 600);
	StrataWarpNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	StrataWarpNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
	StrataWarpNoise.SetFrequency(StrataParameters.TerraceWarpFrequency);
	StrataWarpNoise.SetFractalOctaves(3);
	StrataWarpNoise.SetFractalLacunarity(2.0f);
	StrataWarpNoise.SetFractalGain(0.5f);

	// 8. Climate (Atmospheric Humidity & Planetary Temperature)
	SetupSimplexFBM(HumidityNoise, { BiomeParameters.HumidityFrequency, BiomeParameters.HumidityOctaves, 2.0f, 0.5f, 1.0f }, 700);
	SetupSimplexFBM(TempNoise, { BiomeParameters.TemperatureFrequency, 3, 2.0f, 0.5f, 1.0f }, 800);

	// Precalculate theoretical elevation range for normalization
	const float TrueMinHeight = -0.65f * ContinentalLayer.Amplitude;
	const float TrueMaxHeight = 0.5f * ContinentalLayer.Amplitude
		+ MountainLayer.Amplitude
		+ HillLayer.Amplitude
		+ DetailLayer.Amplitude;

	const float NormScale = FMath::Max(0.0001f, HeightNormalizationScale);
	CachedNormalizedMin = TrueMinHeight * NormScale;
	CachedNormalizedMax = TrueMaxHeight * NormScale;
	if (FMath::IsNearlyEqual(CachedNormalizedMax, CachedNormalizedMin))
	{
		CachedNormalizedMax = CachedNormalizedMin + 1.0f;
	}
}

void FCosmicErosionMultifractalNoiseStrategy::EvaluatePoint(
	const FVector& NoiseDir,
	float& OutHeight,
	FLinearColor& OutColor) const
{
	const float X = NoiseDir.X;
	const float Y = NoiseDir.Y;
	const float Z = NoiseDir.Z;

	// 1. Tectonic Crustal Warping (Shearing & Plate Folding)
	float WarpedX = X;
	float WarpedY = Y;
	float WarpedZ = Z;
	if (MultifractalParameters.TectonicWarpAmplitude > KINDA_SMALL_NUMBER)
	{
		TectonicWarpNoise.DomainWarp(WarpedX, WarpedY, WarpedZ);
	}

	// 2. Continental Landmass & Hypsometric Curve
	const float ContinentRaw = ContinentalNoise.GetNoise(WarpedX, WarpedY, WarpedZ);
	const float Continent = (ContinentRaw + 1.0f) * 0.5f; // [0, 1]

	// Hypsometric curve separating ocean basin from continental landmass (sea level at ~0.46)
	const float OceanMask = 1.0f - FMath::SmoothStep(0.44f, 0.48f, Continent);
	const float LandMask = 1.0f - OceanMask;

	// Base continental shelf elevation
	const float BaseHeight = (Continent - 0.46f) * ContinentalLayer.Amplitude;

	// 3. Heterogeneous Multifractal Relief (Musgrave Hybrid Model)
	// Mountains: Ridged multifractal where ridges become knife-sharp peaks
	const float MountainRaw = MountainNoise.GetNoise(WarpedX, WarpedY, WarpedZ);
	float MountainVal = FMath::Max(0.0f, MountainRaw);

	// Apply peak sharpness exponent to pinch ridges into alpine arêtes
	if (MultifractalParameters.PeakSharpness > 1.0f)
	{
		MountainVal = FMath::Pow(MountainVal, MultifractalParameters.PeakSharpness);
	}
	const float MountainHeight = MountainVal * MountainLayer.Amplitude * LandMask;

	// Rolling hills and sedimentary basin relief
	const float HillRaw = HillNoise.GetNoise(WarpedX, WarpedY, WarpedZ);
	const float HillVal = (HillRaw + 1.0f) * 0.5f;
	const float HillHeight = (HillVal - 0.35f) * HillLayer.Amplitude * LandMask;

	// Musgrave Heterogeneity factor:
	// Lowland plains and coastal basins have low roughness (thick alluvium/sediment deposition).
	// Mountain crests and tectonic uplifts have high roughness (fractured exposed bedrock).
	const float ElevationNormalizedEstimate = FMath::Clamp(
		(BaseHeight + MountainHeight) / FMath::Max(1.0f, ContinentalLayer.Amplitude + MountainLayer.Amplitude),
		0.0f,
		1.0f
	);
	const float HeteroRoughness = FMath::Clamp(
		FMath::Lerp(MultifractalParameters.LowlandSmoothingFactor, 1.0f, FMath::SmoothStep(0.15f, 0.70f, ElevationNormalizedEstimate + MountainVal * 0.5f)),
		0.0f,
		1.0f
	);

	// Fine detail layer dynamically scaled by local Musgrave roughness
	const float DetailRaw = DetailNoise.GetNoise(WarpedX, WarpedY, WarpedZ);
	const float DetailHeight = DetailRaw * DetailLayer.Amplitude * HeteroRoughness * LandMask;

	// Un-eroded land height
	float LandHeight = BaseHeight + HillHeight + MountainHeight + DetailHeight;

	// 4. Fluvial Drainage Networks & Hydraulic Erosion (Cellular Distance2Sub)
	float DrainageIncision = 0.0f;
	float DrainageProximity = 0.0f;

	if (ErosionParameters.bEnableErosion && LandMask > 0.01f)
	{
		// Distance2Sub returns (Distance2 - Distance1 - 1.0)
		// Along cell boundaries (drainage thalwegs/riverbeds), Distance2 == Distance1, so result is -1.0.
		// Adding 1.0 shifts the river centerline to 0.0.
		const float RawCellular = RiverNoise.GetNoise(X, Y, Z);
		const float DrainageDist = FMath::Max(0.0f, RawCellular + 1.0f);

		const float ValleyWidth = FMath::Max(0.001f, ErosionParameters.FluvialValleyWidth);
		if (DrainageDist < ValleyWidth)
		{
			// Normalized distance to river center [0 = center, 1 = valley rim]
			const float NormDist = DrainageDist / ValleyWidth;

			// Non-linear valley carving profile (power curve creates natural concave fluvial valleys)
			DrainageProximity = 1.0f - FMath::Pow(NormDist, ErosionParameters.FluvialBankSmoothness);

			// Flow accumulation approximation: Rivers gather more drainage mass as they flow downward toward sea level.
			// Valleys cut deeper and wider near base level than at high frozen mountain peaks.
			const float FlowAccumulation = FMath::Clamp(
				1.0f - (LandHeight / FMath::Max(1.0f, MountainLayer.Amplitude + ContinentalLayer.Amplitude)),
				0.25f,
				1.0f
			);

			DrainageIncision = DrainageProximity * ErosionParameters.FluvialCarveDepth * FlowAccumulation * LandMask;
			LandHeight -= DrainageIncision;
		}
	}

	// 5. Sedimentary Strata & Terraced Escarpments (Plateau Shaping)
	if (StrataParameters.bEnableTerraces && LandMask > 0.01f)
	{
		const float StepInterval = FMath::Max(1.0f, StrataParameters.TerraceInterval);

		// Geologic fold warp: Bedding planes are gently folded/tilted across terrain
		const float StrataWarp = StrataWarpNoise.GetNoise(X, Y, Z) * StrataParameters.TerraceWarpIntensity;
		const float WarpedElevation = LandHeight + StrataWarp;

		// Phase within current stratum band
		const float NormalizedStep = WarpedElevation / StepInterval;
		const float StratumIndex = FMath::FloorToFloat(NormalizedStep);
		const float FractionalPhase = NormalizedStep - StratumIndex; // [0, 1)

		// Sine-harmonic plateau shaping:
		// Creates wide, flat plateau tops at the middle/end of stratum and steep cliff scarps at boundaries.
		// C-infinity continuous, zero derivative discontinuities, no mesh pinched normals.
		const float Sharpness = FMath::Clamp(StrataParameters.TerraceSharpness, 0.0f, 0.95f);
		const float TwoPi = 6.28318530718f;
		const float ShapedPhase = FractionalPhase
			- (Sharpness / TwoPi) * FMath::Sin(TwoPi * FractionalPhase)
			+ (Sharpness * 0.15f / (TwoPi * 2.0f)) * FMath::Sin(TwoPi * 2.0f * FractionalPhase);

		const float TerracedElevation = (StratumIndex + ShapedPhase) * StepInterval - StrataWarp;

		// Terraces are most pronounced on dry plateaus, badlands, and canyon walls;
		// they attenuate slightly on extreme snow peaks and submerged ocean floors.
		const float PlateauZoneMask = FMath::SmoothStep(0.0f, 0.35f, LandMask) * FMath::Clamp(StrataParameters.TerraceWeight, 0.0f, 1.0f);
		LandHeight = FMath::Lerp(LandHeight, TerracedElevation, PlateauZoneMask);

		// 6. Thermal Erosion & Talus Scree Slope Approximation
		// At the base of steep terrace scarps (FractionalPhase near 0), gravity deposits loose weathered scree.
		// We model talus accumulation as an exponential deposition apron rounding the concave toe of the cliff.
		if (ErosionParameters.TalusDecayStrength > 0.01f && FractionalPhase < 0.25f)
		{
			const float TalusPhase = FractionalPhase / 0.25f;
			const float TalusApron = FMath::Exp(-3.0f * TalusPhase) * (1.0f - TalusPhase);
			const float TalusDisplacement = TalusApron * (StepInterval * 0.18f) * ErosionParameters.TalusDecayStrength * PlateauZoneMask;
			LandHeight += TalusDisplacement;
		}
	}

	// 7. Ocean Basin & Final Elevation Blend
	// Deep ocean floor with oceanic abyssal plain micro-topography
	const float OceanDepth = -ContinentalLayer.Amplitude * 0.65f + (ContinentRaw * 0.10f * ContinentalLayer.Amplitude);
	const float FinalHeight = FMath::Lerp(LandHeight, OceanDepth, OceanMask);
	OutHeight = FinalHeight;

	// 8. Altitude Normalization
	const float Denom = CachedNormalizedMax - CachedNormalizedMin;
	float AltitudeNormalized = (FinalHeight - CachedNormalizedMin) / Denom;
	AltitudeNormalized = FMath::Clamp(AltitudeNormalized, 0.0f, 1.0f);

	// 9. Planetary Temperature (Latitude Gradient + Altitude Lapse Rate)
	const float Latitude = FMath::Abs(Z); // 0 at equator, 1 at poles
	const float BaseTemp = 1.0f - (Latitude * BiomeParameters.LatitudeEffect);
	const float TempVar = TempNoise.GetNoise(X, Y, Z) * 0.18f;
	const float SurfaceTemp = FMath::Clamp(BaseTemp + TempVar, 0.0f, 1.0f);

	// Lapse rate: Temperature drops with elevation (causes glaciers and snow on mountain peaks)
	const float VisualTemp = FMath::Clamp(
		SurfaceTemp - (AltitudeNormalized * BiomeParameters.AltitudeTemperaturePenalty),
		0.0f,
		1.0f
	);

	// 10. Atmospheric Humidity & Drainage Basin Convergence
	const float RawHum = HumidityNoise.GetNoise(X, Y, Z);
	float BaseHumidity = (RawHum + 1.0f) * 0.5f;

	// Apply contrast and offset
	BaseHumidity = FMath::Clamp(
		(BaseHumidity + BiomeParameters.HumidityOffset - 0.5f) * BiomeParameters.HumidityContrast + 0.5f,
		0.0f,
		1.0f
	);

	// Hydrological Drainage Convergence:
	// Water flows into valleys, river corridors, and sedimentary basins.
	// River channels and low-elevation alluvial plains receive a substantial moisture boost.
	const float RiverMoistureBonus = DrainageProximity * ErosionParameters.DrainageMoistureConvergence * LandMask;
	const float LowlandMoistureBonus = (1.0f - AltitudeNormalized) * 0.15f * LandMask;

	// Oceans are 100% saturated
	float Humidity = FMath::Clamp(
		BaseHumidity * LandMask + RiverMoistureBonus + LowlandMoistureBonus + OceanMask * 1.0f,
		0.0f,
		1.0f
	);

	// 11. Mandatory Final Color Assignment
	OutColor = FLinearColor(
		AltitudeNormalized,  // R
		VisualTemp,          // G
		Humidity,            // B     
		1.0f                 // A 
	);
}
