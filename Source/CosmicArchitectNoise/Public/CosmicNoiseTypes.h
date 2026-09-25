// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "CosmicNoiseTypes.generated.h"

UENUM(BlueprintType)
enum class ECosmicNoiseType : uint8
{
    Perlin,
    Simplex,
    Cellular,
    Value,
    Ridged
};

UENUM(BlueprintType)
enum class ECosmicFractalType : uint8
{
    None,
    FBM,
    Ridged, 
    PingPong
};

/**
 * 
 */
USTRUCT(Blueprintable, BlueprintType)
struct COSMICARCHITECTNOISE_API FCosmicNoiseLayer 
{
	GENERATED_BODY()
	
public:
    UPROPERTY(EditAnywhere, Category = "NoiseLayer", BlueprintReadWrite)
    ECosmicNoiseType NoiseType = ECosmicNoiseType::Simplex;

    UPROPERTY(EditAnywhere, Category = "NoiseLayer", BlueprintReadWrite)
    ECosmicFractalType FractalType = ECosmicFractalType::FBM;

    UPROPERTY(EditAnywhere, Category = "NoiseLayer", BlueprintReadWrite, meta = (ClampMin = "0"))
    float Frequency = 0.001f;

    UPROPERTY(EditAnywhere, Category = "NoiseLayer", BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "12"))
    int32 Octaves = 5;

    UPROPERTY(EditAnywhere, Category = "NoiseLayer", BlueprintReadWrite, meta = (ClampMin = "0"))
    float Lacunarity = 2.0f;

    UPROPERTY(EditAnywhere, Category = "NoiseLayer", BlueprintReadWrite, meta = (ClampMin = "0"))
    float Persistence = 0.5f;

    UPROPERTY(EditAnywhere, Category = "NoiseLayer", BlueprintReadWrite, meta = (ClampMin = "0"))
    float Amplitude = 1.0f;

};

USTRUCT(Blueprintable, BlueprintType)
struct COSMICARCHITECTNOISE_API FCosmicNoiseDataLayer
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "NoiseLayer", BlueprintReadWrite, meta = (ClampMin = "0"))
    float Frequency = 0.001f;

    UPROPERTY(EditAnywhere, Category = "NoiseLayer", BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "12"))
    int32 Octaves = 5;

    UPROPERTY(EditAnywhere, Category = "NoiseLayer", BlueprintReadWrite, meta = (ClampMin = "0"))
    float Lacunarity = 2.0f;

    UPROPERTY(EditAnywhere, Category = "NoiseLayer", BlueprintReadWrite, meta = (ClampMin = "0"))
    float Persistence = 0.5f;

    UPROPERTY(EditAnywhere, Category = "NoiseLayer", BlueprintReadWrite, meta = (ClampMin = "0"))
    float Amplitude = 1.0f;
};

UENUM()
enum class ECosmicBiomeType : uint8
{
    TemperateForest    UMETA(DisplayName = "Temperate Forest"),
    Rainforest         UMETA(DisplayName = "Rainforest"),
    Desert             UMETA(DisplayName = "Desert"),
    Tundra             UMETA(DisplayName = "Tundra"),
    Taiga              UMETA(DisplayName = "Taiga"),
    Savannah           UMETA(DisplayName = "Savannah"),
    Grassland          UMETA(DisplayName = "Grassland"),
    Swamp              UMETA(DisplayName = "Swamp"),
    Volcanic           UMETA(DisplayName = "Volcanic"),
    Alien              UMETA(DisplayName = "Alien"),
    Ocean              UMETA(DisplayName = "Ocean"),
    Ice                UMETA(DisplayName = "Ice"),
    Cratered           UMETA(DisplayName = "Cratered Moon")
};

USTRUCT(BlueprintType)
struct COSMICARCHITECTNOISE_API FCosmicNoiseSimpleParameters
{
    GENERATED_BODY()

    /* MODE SWITCH */
    UPROPERTY(EditAnywhere, Category = "Mode")
    bool bUseAdvancedSettings = false;

    /* SIMPLE MODE */
    UPROPERTY(EditAnywhere, Category = "Simple", meta = (EditCondition = "!bUseAdvancedSettings", ClampMin = "0"))
    float MaxMountainHeight = 3000.0f;

    UPROPERTY(EditAnywhere, Category = "Simple", meta = (EditCondition = "!bUseAdvancedSettings", ClampMin = "0", ClampMax = "1"))
    float Mountainous = 0.6f;

    UPROPERTY(EditAnywhere, Category = "Simple", meta = (EditCondition = "!bUseAdvancedSettings", ClampMin = "0", ClampMax = "1"))
    float Roughness = 0.4f;

    UPROPERTY(EditAnywhere, Category = "Simple", meta = (EditCondition = "!bUseAdvancedSettings", ClampMin = "0", ClampMax = "1"))
    float Detail = 0.7f;

    UPROPERTY(EditAnywhere, Category = "Simple", meta = (EditCondition = "!bUseAdvancedSettings", ClampMin = "0", ClampMax = "1"))
    float Smoothness = 0.5f;

    UPROPERTY(EditAnywhere, Category = "Simple", meta = (EditCondition = "!bUseAdvancedSettings"))
    ECosmicBiomeType BiomeType = ECosmicBiomeType::Desert;
};

USTRUCT(BlueprintType)
struct COSMICARCHITECTNOISE_API FCosmicNoiseCraterParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Craters", meta = (ClampMin = "0.0001", ClampMax = "100"))
    float CraterFrequency = 2.0f;

    UPROPERTY(EditAnywhere, Category = "Craters")
    float CraterDepth = 300.0f;

    UPROPERTY(EditAnywhere, Category = "Craters", meta = (ClampMin = "1", ClampMax = "8"))
    int32 CraterOctaves = 3;

    UPROPERTY(EditAnywhere, Category = "Craters", meta = (ClampMin = "0.05", ClampMax = "1.0"))
    float CraterDensity = 0.55f;

    UPROPERTY(EditAnywhere, Category = "Craters", meta = (ClampMin = "0", ClampMax = "2"))
    float CraterRadiusMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Craters", meta = (ClampMin = "0", ClampMax = "2"))
    float CraterRimHeight = 0.4f;

    UPROPERTY(EditAnywhere, Category = "Craters", meta = (ClampMin = "0.1", ClampMax = "20"))
    float CraterRimSharpness = 2.5f;

    UPROPERTY(EditAnywhere, Category = "Craters", meta = (ClampMin = "0", ClampMax = "1"))
    float CraterFloorHeight = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Craters", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CentralPeakHeight = 0.35f;

    UPROPERTY(EditAnywhere, Category = "Craters", meta = (ClampMin = "0.05", ClampMax = "0.5"))
    float CentralPeakRadius = 0.22f;

    UPROPERTY(EditAnywhere, Category = "Craters", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float EjectaStrength = 0.45f;

    UPROPERTY(EditAnywhere, Category = "Craters", meta = (ClampMin = "0", ClampMax = "1"))
    float CraterDistortion = 0.15f;

    UPROPERTY(EditAnywhere, Category = "Craters", meta = (ClampMin = "0", ClampMax = "10"))
    float CraterLacunarity = 2.5f;

    UPROPERTY(EditAnywhere, Category = "Craters", meta = (ClampMin = "0", ClampMax = "1"))
    float CraterPersistence = 0.5f;

    UPROPERTY(EditAnywhere, Category = "Craters", meta = (ClampMin = "0", ClampMax = "1"))
    float CraterNoiseBreakup = 0.2f;

    UPROPERTY(EditAnywhere, Category = "Craters - Maria Basins", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MariaThreshold = 0.35f;
};

USTRUCT(BlueprintType)
struct COSMICARCHITECTNOISE_API FCosmicNoiseBiomeParameters
{
    GENERATED_BODY()

    /** Frequency for temperature noise (0.001-0.02) */
    UPROPERTY(EditAnywhere, Category = "Biome", meta = (ClampMin = "0.0001"))
    float TemperatureFrequency = 0.005f;

    /** Latitude effect intensity (0 = noise only, 1 = strong polar gradient) */
    UPROPERTY(EditAnywhere, Category = "Biome", meta = (ClampMin = "0", ClampMax = "2"))
    float LatitudeEffect = 1.0f;

    /** Temperature penalty for altitude (0-1) */
    UPROPERTY(EditAnywhere, Category = "Biome", meta = (ClampMin = "0", ClampMax = "1"))
    float AltitudeTemperaturePenalty = 0.6f;

    /** Frequency for humidity noise (0.001-0.1) */
    UPROPERTY(EditAnywhere, Category = "Biome", meta = (ClampMin = "0.0001"))
    float HumidityFrequency = 0.015f;

    /** Octaves for humidity (more = more detailed clouds) */
    UPROPERTY(EditAnywhere, Category = "Biome", meta = (ClampMin = "1", ClampMax = "8"))
    int32 HumidityOctaves = 5;

    /** Humidity contrast (1 = normal, >1 = more extreme) */
    UPROPERTY(EditAnywhere, Category = "Biome", meta = (ClampMin = "0.5", ClampMax = "3"))
    float HumidityContrast = 1.5f;

    /** Humidity offset (to shift the range) */
    UPROPERTY(EditAnywhere, Category = "Biome", meta = (ClampMin = "-1", ClampMax = "1"))
    float HumidityOffset = -0.5f;
};

USTRUCT(BlueprintType)
struct COSMICARCHITECTNOISE_API FCosmicNoiseDomainWarpParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "DomainWarp", BlueprintReadWrite)
    bool bUseDomainWarp = false;

    UPROPERTY(EditAnywhere, Category = "DomainWarp", BlueprintReadWrite)
    float DomainWarpStrength = 0.25f;

    UPROPERTY(EditAnywhere, Category = "DomainWarp", BlueprintReadWrite)
    float DomainWarpFrequency = 1.0f;

    UPROPERTY(EditAnywhere, Category = "DomainWarp", BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "8"))
    int32 DomainWarpOctaves = 3;
};

USTRUCT(BlueprintType)
struct COSMICARCHITECTNOISE_API FCosmicOrographicParameters
{
    GENERATED_BODY()

    /** Enable orographic precipitation and rain shadow simulation */
    UPROPERTY(EditAnywhere, Category = "Orographic Climatology", BlueprintReadWrite)
    bool bEnableOrographicEffect = true;

    /** If true, uses realistic planetary atmospheric circulation cells (Hadley / Ferrel / Polar reversals). If false, uses PrevailingWindDirection. */
    UPROPERTY(EditAnywhere, Category = "Orographic Climatology", BlueprintReadWrite)
    bool bUsePlanetaryZonalWinds = true;

    /** Global prevailing wind direction if bUsePlanetaryZonalWinds is false */
    UPROPERTY(EditAnywhere, Category = "Orographic Climatology", BlueprintReadWrite)
    FVector PrevailingWindDirection = FVector(1.0f, 0.0f, 0.0f);

    /** Boost to precipitation / humidity on windward mountain slopes (0.0 to 5.0) */
    UPROPERTY(EditAnywhere, Category = "Orographic Climatology", BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float OrographicLiftStrength = 1.2f;

    /** Drying intensity on leeward slopes and rain shadow zones behind mountain ranges (0.0 to 5.0) */
    UPROPERTY(EditAnywhere, Category = "Orographic Climatology", BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float RainShadowStrength = 1.5f;

    /** Angular/spatial offset distance along wind vector to sample slope for orographic lift */
    UPROPERTY(EditAnywhere, Category = "Orographic Climatology", BlueprintReadWrite, meta = (ClampMin = "0.001", ClampMax = "0.2"))
    float WindwardSampleOffset = 0.015f;

    /** Angular/spatial offset distance along wind vector to detect upwind mountain barriers for rain shadow */
    UPROPERTY(EditAnywhere, Category = "Orographic Climatology", BlueprintReadWrite, meta = (ClampMin = "0.005", ClampMax = "0.5"))
    float RainShadowDistance = 0.06f;
};

USTRUCT(BlueprintType)
struct COSMICARCHITECTNOISE_API FCosmicMultiNoiseParameters
{
    GENERATED_BODY()

    /** Sea level threshold in continentalness [0, 1]. Values below this are submerged oceans. Default: 0.45 */
    UPROPERTY(EditAnywhere, Category = "MultiNoise - Continentalness", BlueprintReadWrite, meta = (ClampMin = "0.1", ClampMax = "0.9"))
    float SeaLevelThreshold = 0.45f;

    /** Multiplier for deep oceanic crust and trenches relative to continental amplitude */
    UPROPERTY(EditAnywhere, Category = "MultiNoise - Continentalness", BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "3.0"))
    float OceanDepthScale = 0.7f;

    /** Width of continental shelf transition zone around sea level */
    UPROPERTY(EditAnywhere, Category = "MultiNoise - Continentalness", BlueprintReadWrite, meta = (ClampMin = "0.005", ClampMax = "0.4"))
    float ContinentalShelfWidth = 0.08f;

    /** Base elevation boost for high continental interiors and tectonic plateaus */
    UPROPERTY(EditAnywhere, Category = "MultiNoise - Continentalness", BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float InlandPlateauBoost = 0.35f;

    /** How strongly high erosion flattens the terrain relief (0.0 = totally flat peneplain, 1.0 = no flattening) */
    UPROPERTY(EditAnywhere, Category = "MultiNoise - Erosion", BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float HighErosionFlattening = 0.15f;

    /** Contrast exponent for erosion distribution curve */
    UPROPERTY(EditAnywhere, Category = "MultiNoise - Erosion", BlueprintReadWrite, meta = (ClampMin = "0.5", ClampMax = "4.0"))
    float ErosionContrast = 1.4f;

    /** Mountain peak sharpness exponent (PV^Sharpness). Higher values create knife-edge alpine horns */
    UPROPERTY(EditAnywhere, Category = "MultiNoise - Peaks & Valleys", BlueprintReadWrite, meta = (ClampMin = "0.5", ClampMax = "5.0"))
    float MountainPeakSharpness = 2.2f;

    /** Canyon and valley depth multiplier relative to PeaksValleysLayer amplitude */
    UPROPERTY(EditAnywhere, Category = "MultiNoise - Peaks & Valleys", BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "3.0"))
    float ValleyDepthMultiplier = 0.85f;

    /** Terracing / geological stepped strata strength for plateaus and canyons (0.0 = disabled) */
    UPROPERTY(EditAnywhere, Category = "MultiNoise - Geological Features", BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float TerracingStrength = 0.0f;

    /** Number of terrace steps per 1000m */
    UPROPERTY(EditAnywhere, Category = "MultiNoise - Geological Features", BlueprintReadWrite, meta = (ClampMin = "1.0", ClampMax = "20.0"))
    float TerraceSteps = 6.0f;
};

