// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CosmicNoiseClass.h"
#include "CosmicNoiseTypes.h"
#include "CosmicErosionMultifractalNoiseSettings.generated.h"

/**
 * Parameters controlling sedimentary strata and stepped terrace formations (plateaus, mesas, canyons).
 */
USTRUCT(BlueprintType)
struct COSMICARCHITECTNOISE_API FCosmicStrataTerraceParameters
{
	GENERATED_BODY()

	/** Whether to enable sedimentary terracing and plateau shaping. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strata & Terraces")
	bool bEnableTerraces = true;

	/**
	 * Vertical spacing (altitude height difference) between consecutive geological terrace benches / strata shelves.
	 * Lower values = more tightly packed strata bands. Higher values = massive mesa steps and towering cliffs.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strata & Terraces", meta = (ClampMin = "10.0", ClampMax = "1000.0"))
	float TerraceInterval = 140.0f;

	/**
	 * Sharpness of the terrace transition (0.0 = completely smooth slope, 0.95 = near-vertical scarp cliffs with flat plateaus).
	 * Modulates the continuous sine-harmonic shaping curve.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strata & Terraces", meta = (ClampMin = "0.0", ClampMax = "0.95"))
	float TerraceSharpness = 0.82f;

	/**
	 * Global weight/blend factor of the terrace profile onto the raw terrain height (0.0 to 1.0).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strata & Terraces", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TerraceWeight = 0.70f;

	/**
	 * Intensity of geological domain folding / warping applied to strata layers, simulating tilted sedimentary beds and anticlines.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strata & Terraces", meta = (ClampMin = "0.0", ClampMax = "200.0"))
	float TerraceWarpIntensity = 35.0f;

	/**
	 * Spatial frequency for geological folding and strata warping.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strata & Terraces", meta = (ClampMin = "0.0001", ClampMax = "0.1"))
	float TerraceWarpFrequency = 0.0025f;
};

/**
 * Parameters controlling fluvial drainage and cellular hydraulic/thermal erosion approximation.
 */
USTRUCT(BlueprintType)
struct COSMICARCHITECTNOISE_API FCosmicErosionParameters
{
	GENERATED_BODY()

	/** Whether to enable hydraulic river carving and thermal erosion approximations. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion & Drainage")
	bool bEnableErosion = true;

	/**
	 * Maximum depth carved into the bedrock along major river drainage arteries.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion & Drainage", meta = (ClampMin = "0.0", ClampMax = "2000.0"))
	float FluvialCarveDepth = 220.0f;

	/**
	 * Width threshold for river drainage valleys (based on Voronoi Distance2Sub).
	 * Controls the cross-sectional influence zone of watercourses.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion & Drainage", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float FluvialValleyWidth = 0.25f;

	/**
	 * Valley cross-section profile exponent (0.5 = steep V-notch gorge, 1.0 = parabolic fluvial valley, 2.0 = broad U-shaped valley).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion & Drainage", meta = (ClampMin = "0.2", ClampMax = "3.0"))
	float FluvialBankSmoothness = 0.85f;

	/**
	 * Cellular Voronoi jitter modifier. 1.0 = fully natural irregular dendritic drainage; lower = more aligned grid.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion & Drainage", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float DrainageJitter = 0.95f;

	/**
	 * Strength of thermal scree/talus accumulation at the foot of steep escarpments and canyon walls.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion & Drainage", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TalusDecayStrength = 0.35f;

	/**
	 * Moisture convergence factor along drainage networks. Increases soil humidity and lush vegetation in valleys and riverbanks.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion & Drainage", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DrainageMoistureConvergence = 0.45f;
};

/**
 * Parameters controlling heterogeneous multifractal terrain generation (Musgrave model).
 */
USTRUCT(BlueprintType)
struct COSMICARCHITECTNOISE_API FCosmicMultifractalParameters
{
	GENERATED_BODY()

	/**
	 * Exponent power applied to mountain ridged peaks to pinch them into razor-sharp arêtes and alpine horns.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multifractal", meta = (ClampMin = "1.0", ClampMax = "4.0"))
	float PeakSharpness = 1.7f;

	/**
	 * Controls how much high-frequency detail is suppressed in lowlands and alluvial plains (0.0 = complete sediment smoothing, 1.0 = uniform roughness everywhere).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multifractal", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowlandSmoothingFactor = 0.15f;

	/**
	 * Weighted strength for FastNoiseLite ridged multifractal calculations. Controls inter-octave amplitude feedback.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multifractal", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FractalWeightedStrength = 0.85f;

	/**
	 * Tectonic domain warp frequency used to shear mountain chains into realistic tectonic fold belts.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multifractal", meta = (ClampMin = "0.0001", ClampMax = "0.05"))
	float TectonicWarpFrequency = 0.0015f;

	/**
	 * Amplitude of tectonic domain warp displacement.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multifractal", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float TectonicWarpAmplitude = 0.35f;
};

/**
 * Advanced procedural noise configuration asset based on Heterogeneous Multifractals (Musgrave),
 * Sedimentary Strata Terraces, and Fluvial/Thermal Erosion Approximations.
 */
UCLASS(BlueprintType)
class COSMICARCHITECTNOISE_API UCosmicErosionMultifractalNoiseSettings : public UCosmicNoiseClass
{
	GENERATED_BODY()

public:
	UCosmicErosionMultifractalNoiseSettings();

	/** Procedural generation seed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
	int32 Seed = 1337;

	/** Scale used to normalize final terrain height in visualization and biome mapping. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HeightNormalizationScale = 1.0f;

	/** Main noise layer defining macro continental plates and ocean basins. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Layers")
	FCosmicNoiseDataLayer ContinentalLayer;

	/** Mountain noise layer using hybrid ridged multifractal generation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Layers")
	FCosmicNoiseDataLayer MountainLayer;

	/** Secondary relief and rolling hill layer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Layers")
	FCosmicNoiseDataLayer HillLayer;

	/** Fine high-frequency detail layer (dynamically modulated by Musgrave roughness). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Layers")
	FCosmicNoiseDataLayer DetailLayer;

	/** Cellular drainage network layer for fluvial river carving. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Layers")
	FCosmicNoiseDataLayer RiverLayer;

	/** Sedimentary strata and stepped terrace configuration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geological Features")
	FCosmicStrataTerraceParameters StrataParameters;

	/** Fluvial drainage network and hydraulic/thermal erosion settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geological Features")
	FCosmicErosionParameters ErosionParameters;

	/** Heterogeneous multifractal (Musgrave) and tectonic distortion settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geological Features")
	FCosmicMultifractalParameters MultifractalParameters;

	/** Planetary climate and biome parameters (temperature, humidity, lapse rate). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Settings")
	FCosmicNoiseBiomeParameters BiomeParameters;

	/** Creates and initializes the erosion multifractal noise strategy. */
	virtual TSharedPtr<ICosmicNoiseStrategy> CreateStrategy() const override;
};
