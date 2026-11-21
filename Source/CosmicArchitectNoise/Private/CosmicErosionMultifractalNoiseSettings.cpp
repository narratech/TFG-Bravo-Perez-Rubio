// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicErosionMultifractalNoiseSettings.h"
#include "CosmicErosionMultifractalNoiseStrategy.h"

UCosmicErosionMultifractalNoiseSettings::UCosmicErosionMultifractalNoiseSettings()
{
	Seed = 1337;
	HeightNormalizationScale = 1.0f;

	// 1. Continental Macro Layer: Defines oceans, continental shelves and broad landmasses on unit sphere
	ContinentalLayer.Frequency = 1.0f;
	ContinentalLayer.Octaves = 6;
	ContinentalLayer.Lacunarity = 2.0f;
	ContinentalLayer.Persistence = 0.5f;
	ContinentalLayer.Amplitude = 36000.0f;

	// 2. Mountain Tectonic Layer: Hybrid ridged multifractal creating alpine chains
	MountainLayer.Frequency = 10.0f;
	MountainLayer.Octaves = 7;
	MountainLayer.Lacunarity = 2.1f;
	MountainLayer.Persistence = 0.55f;
	MountainLayer.Amplitude = 30000.0f;

	// 3. Hill & Basin Layer: Secondary rolling relief and sediment base
	HillLayer.Frequency = 24.0f;
	HillLayer.Octaves = 5;
	HillLayer.Lacunarity = 2.0f;
	HillLayer.Persistence = 0.48f;
	HillLayer.Amplitude = 7000.0f;

	// 4. Detail Layer: Micro-relief and rocky surface facets
	DetailLayer.Frequency = 500.0f;
	DetailLayer.Octaves = 5;
	DetailLayer.Lacunarity = 2.0f;
	DetailLayer.Persistence = 0.45f;
	DetailLayer.Amplitude = 1000.0f;

	// 5. River & Fluvial Drainage Layer: Cellular Voronoi dendritic drainage network
	RiverLayer.Frequency = 6.0f;
	RiverLayer.Octaves = 1;
	RiverLayer.Lacunarity = 2.0f;
	RiverLayer.Persistence = 0.5f;
	RiverLayer.Amplitude = 2400.0f;

	// 6. Geological Strata & Stepped Terracing Parameters
	StrataParameters.bEnableTerraces = true;
	StrataParameters.TerraceInterval = 2200.0f;
	StrataParameters.TerraceSharpness = 0.78f;
	StrataParameters.TerraceWeight = 0.42f;
	StrataParameters.TerraceWarpIntensity = 600.0f;
	StrataParameters.TerraceWarpFrequency = 3.0f;

	// 7. Fluvial Drainage & Erosion Parameters
	ErosionParameters.bEnableErosion = true;
	ErosionParameters.FluvialCarveDepth = 2400.0f;
	ErosionParameters.FluvialValleyWidth = 0.22f;
	ErosionParameters.FluvialBankSmoothness = 0.95f;
	ErosionParameters.DrainageJitter = 0.95f;
	ErosionParameters.TalusDecayStrength = 0.35f;
	ErosionParameters.DrainageMoistureConvergence = 0.45f;

	// 8. Heterogeneous Multifractal Parameters
	MultifractalParameters.PeakSharpness = 1.65f;
	MultifractalParameters.LowlandSmoothingFactor = 0.15f;
	MultifractalParameters.FractalWeightedStrength = 0.85f;
	MultifractalParameters.TectonicWarpFrequency = 1.5f;
	MultifractalParameters.TectonicWarpAmplitude = 0.25f;

	// 9. Planetary Biome Parameters
	BiomeParameters.TemperatureFrequency = 4.0f;
	BiomeParameters.LatitudeEffect = 0.55f;
	BiomeParameters.AltitudeTemperaturePenalty = 0.75f;
	BiomeParameters.HumidityFrequency = 45.0f;
	BiomeParameters.HumidityOctaves = 4;
	BiomeParameters.HumidityContrast = 1.4f;
	BiomeParameters.HumidityOffset = 0.0f;
}

TSharedPtr<ICosmicNoiseStrategy> UCosmicErosionMultifractalNoiseSettings::CreateStrategy() const
{
	TSharedPtr<FCosmicErosionMultifractalNoiseStrategy> Strategy = MakeShared<FCosmicErosionMultifractalNoiseStrategy>();
	Strategy->Initialize(*this);
	return Strategy;
}
