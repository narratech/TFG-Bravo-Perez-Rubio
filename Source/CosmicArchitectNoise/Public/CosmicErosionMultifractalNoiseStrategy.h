// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ICosmicNoiseStrategy.h"
#include "CosmicNoiseTypes.h"
#include "CosmicErosionMultifractalNoiseSettings.h"
#include "ThirdParty/FastNoiseLite.h"

/**
 * Advanced procedural noise strategy combining:
 * 1. Heterogeneous Multifractals (Musgrave model with variable roughness based on elevation and slope).
 * 2. Sedimentary Strata and Terraced Escarpments (sine-harmonic plateau shaping with geological fold warping).
 * 3. Fluvial Drainage Networks and Hydraulic/Thermal Erosion Approximations (Cellular Distance2Sub Voronoi).
 * 4. Realistic Biome Modeling with Moisture Convergence and Altitude Lapse Rates.
 */
class COSMICARCHITECTNOISE_API FCosmicErosionMultifractalNoiseStrategy : public ICosmicNoiseStrategy
{
public:
	FCosmicErosionMultifractalNoiseStrategy() = default;
	virtual ~FCosmicErosionMultifractalNoiseStrategy() override = default;

	/**
	 * Initializes all noise generators and stores configuration parameters from the settings asset.
	 */
	void Initialize(const UCosmicErosionMultifractalNoiseSettings& InSettings);

	/**
	 * Evaluates a 3D direction vector on the planetary sphere to compute terrain elevation
	 * and biome parameters (Altitude, Visual Temperature, Humidity).
	 *
	 * @param NoiseDir Normalized 3D point on the planet's unit sphere.
	 * @param OutHeight Output height displacement relative to sea level.
	 * @param OutColor Output FLinearColor encoding:
	 *                 R = AltitudeNormalized
	 *                 G = VisualTemp
	 *                 B = Humidity
	 *                 A = 1.0f
	 */
	virtual void EvaluatePoint(
		const FVector& NoiseDir,
		float& OutHeight,
		FLinearColor& OutColor
	) const override;

	// Configuration getters
	int32 GetSeed() const { return Seed; }
	float GetHeightNormalizationScale() const { return HeightNormalizationScale; }

protected:
	int32 Seed = 1337;
	float HeightNormalizationScale = 1.0f;

	FCosmicNoiseDataLayer ContinentalLayer;
	FCosmicNoiseDataLayer MountainLayer;
	FCosmicNoiseDataLayer HillLayer;
	FCosmicNoiseDataLayer DetailLayer;
	FCosmicNoiseDataLayer RiverLayer;

	FCosmicStrataTerraceParameters StrataParameters;
	FCosmicErosionParameters ErosionParameters;
	FCosmicMultifractalParameters MultifractalParameters;
	FCosmicNoiseBiomeParameters BiomeParameters;

	/** Macro continental landmass noise (Simplex fBm) */
	FastNoiseLite ContinentalNoise;

	/** Mountain range noise (OpenSimplex2 Ridged with Musgrave Weighted Strength) */
	FastNoiseLite MountainNoise;

	/** Secondary hill and plateau base noise (OpenSimplex2 fBm) */
	FastNoiseLite HillNoise;

	/** Fine detail and micro-relief noise (ValueCubic fBm) */
	FastNoiseLite DetailNoise;

	/** Fluvial drainage network noise (Cellular Distance2Sub Voronoi) */
	FastNoiseLite RiverNoise;

	/** Tectonic crustal domain warping generator (OpenSimplex2 Domain Warp) */
	FastNoiseLite TectonicWarpNoise;

	/** Geological strata folding and bedding plane noise (Simplex fBm) */
	FastNoiseLite StrataWarpNoise;

	/** Macro atmospheric humidity noise (Simplex fBm) */
	FastNoiseLite HumidityNoise;

	/** Planetary temperature variation noise (Simplex fBm) */
	FastNoiseLite TempNoise;

	/** Precomputed theoretical height extrema for fast normalization */
	float CachedNormalizedMin = 0.0f;
	float CachedNormalizedMax = 1.0f;
};
