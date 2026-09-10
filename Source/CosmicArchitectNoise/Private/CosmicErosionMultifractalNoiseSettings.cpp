// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicErosionMultifractalNoiseSettings.h"
#include "CosmicErosionMultifractalNoiseStrategy.h"

UCosmicErosionMultifractalNoiseSettings::UCosmicErosionMultifractalNoiseSettings()
{
	Seed = 1337;
	HeightNormalizationScale = 1.0f;

	// 1. Continental Macro Layer: Defines oceans, continental shelves and broad landmasses
	ContinentalLayer.Frequency = 0.0008f;
	ContinentalLayer.Octaves = 5;
	ContinentalLayer.Lacunarity = 2.0f;
	ContinentalLayer.Persistence = 0.5f;
	ContinentalLayer.Amplitude = 1400.0f;

	// 2. Mountain Tectonic Layer: Hybrid ridged multifractal creating alpine chains
	MountainLayer.Frequency = 0.0022f;
	MountainLayer.Octaves = 6;
	MountainLayer.Lacunarity = 2.15f;
	MountainLayer.Persistence = 0.55f;
	MountainLayer.Amplitude = 2200.0f;

	// 3. Hill & Basin Layer: Secondary rolling relief and sediment base
	HillLayer.Frequency = 0.0035f;
	HillLayer.Octaves = 4;
	HillLayer.Lacunarity = 2.0f;
	HillLayer.Persistence = 0.45f;
	HillLayer.Amplitude = 450.0f;

	// 4. Detail Layer: Micro-relief and rocky surface facets
	DetailLayer.Frequency = 0.015f;
	DetailLayer.Octaves = 4;
	DetailLayer.Lacunarity = 2.0f;
	DetailLayer.Persistence = 0.5f;
	DetailLayer.Amplitude = 80.0f;

	// 5. River & Fluvial Drainage Layer: Cellular Voronoi dendritic drainage network
	RiverLayer.Frequency = 0.0025f;
	RiverLayer.Octaves = 1;
	RiverLayer.Lacunarity = 2.0f;
	RiverLayer.Persistence = 0.5f;
	RiverLayer.Amplitude = 200.0f;

	// 6. Geological Strata & Stepped Terracing Parameters
	StrataParameters.bEnableTerraces = true;
	StrataParameters.TerraceInterval = 140.0f;
	StrataParameters.TerraceSharpness = 0.82f;
	StrataParameters.TerraceWeight = 0.70f;
	StrataParameters.TerraceWarpIntensity = 35.0f;
	StrataParameters.TerraceWarpFrequency = 0.0025f;

	// 7. Fluvial Drainage & Erosion Parameters
	ErosionParameters.bEnableErosion = true;
	ErosionParameters.FluvialCarveDepth = 220.0f;
	ErosionParameters.FluvialValleyWidth = 0.25f;
	ErosionParameters.FluvialBankSmoothness = 0.85f;
	ErosionParameters.DrainageJitter = 0.95f;
	ErosionParameters.TalusDecayStrength = 0.35f;
	ErosionParameters.DrainageMoistureConvergence = 0.45f;

	// 8. Heterogeneous Multifractal Parameters
	MultifractalParameters.PeakSharpness = 1.7f;
	MultifractalParameters.LowlandSmoothingFactor = 0.15f;
	MultifractalParameters.FractalWeightedStrength = 0.85f;
	MultifractalParameters.TectonicWarpFrequency = 0.0015f;
	MultifractalParameters.TectonicWarpAmplitude = 0.35f;

	// 9. Planetary Biome Parameters
	BiomeParameters.TemperatureFrequency = 0.004f;
	BiomeParameters.LatitudeEffect = 0.95f;
	BiomeParameters.AltitudeTemperaturePenalty = 0.65f;
	BiomeParameters.HumidityFrequency = 0.012f;
	BiomeParameters.HumidityOctaves = 4;
	BiomeParameters.HumidityContrast = 1.4f;
	BiomeParameters.HumidityOffset = -0.1f;
}

TSharedPtr<ICosmicNoiseStrategy> UCosmicErosionMultifractalNoiseSettings::CreateStrategy() const
{
	TSharedPtr<FCosmicErosionMultifractalNoiseStrategy> Strategy = MakeShared<FCosmicErosionMultifractalNoiseStrategy>();
	Strategy->Initialize(*this);
	return Strategy;
}
