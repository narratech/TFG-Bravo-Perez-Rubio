// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CosmicPlanet.generated.h"

class UCosmicClipmapComponent;
class UCosmicNoiseClass;
class UCosmicFoliageSpawner;
class UCosmicCollisionComponent;
class UCosmicOceanComponent;
class UCosmicFoliageCollection;
class UMaterialInstance;

/**
 * ACosmicPlanet
 * Main actor representing a procedural planetary body. 
 * Orchestrates terrain generation via Clipmaps, ocean simulation,
 * dynamic collision systems, and large-scale foliage distribution.
 */
UCLASS(HideCategories = (
	Replication, Input, Actor, LOD, Activation, Cooking, Networking,
	Physics, Navigation, Tags, DataLayers, LevelInstance))
	class COSMICARCHITECTRUNTIME_API ACosmicPlanet : public AActor
{
	GENERATED_BODY()

public:

	/** Base planet radius in Kilometers (supports Large World Coordinates). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet")
	double RadiusKm = 1.0;

	/** Root component of the actor hierarchy. */
	UPROPERTY(VisibleAnywhere, Category = "Planet", BlueprintReadOnly)
	USceneComponent* Root;

	/** Terrain management system based on concentric levels of detail (Clipmap). */
	UPROPERTY(VisibleAnywhere, Category = "Planet", BlueprintReadOnly)
	TObjectPtr<UCosmicClipmapComponent> ClipmapComponent;

	/** Manages real-time collision mesh generation around the observer. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet")
	UCosmicCollisionComponent* CollisionComponent;

	/** Component responsible for visual and physical representation of sea level. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet")
	UCosmicOceanComponent* OceanComponent;

	/** Asset defining noise algorithms for terrain relief. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Noise")
	UCosmicNoiseClass* NoiseClass;

	/** Mass instantiation system for vegetation and rocks on the surface. */
	UPROPERTY(VisibleAnywhere, Category = "Planet", BlueprintReadOnly)
	UCosmicFoliageSpawner* FoliageSpawnerComponent;

	// --- MATERIAL COLOR CONFIGURATION ---

	/** Predominant color for mid-altitude zones. */
	UPROPERTY(EditAnywhere, Category = "Materials|Color")
	FColor PlanetMainColor1 = FColor::Red;

	/** Secondary color for terrain chromatic variation. */
	UPROPERTY(EditAnywhere, Category = "Materials|Color")
	FColor PlanetMainColor2 = FColor::Orange;

	/** Tint applied to low temperature areas or deep valleys. */
	UPROPERTY(EditAnywhere, Category = "Materials|Color")
	FColor PlanetColdColor = FColor::White;

	/** Tint applied to peaks or high activity/temperature areas. */
	UPROPERTY(EditAnywhere, Category = "Materials|Color")
	FColor PlanetHotColor = FColor::Red;

	/** Color used to highlight steep slopes and cliffs. */
	UPROPERTY(EditAnywhere, Category = "Materials|Color")
	FColor PlanetSlopeColor = FColor::Black;

	// --- NOISE SCALES ---

	/** Fine terrain detail (Micro-relief). */
	UPROPERTY(EditAnywhere, Category = "Materials|Noise", meta = (ClampMin = "0.01"))
	float NoiseScaleSmall = 1.f;

	/** Medium terrain detail (Hills and formations). */
	UPROPERTY(EditAnywhere, Category = "Materials|Noise", meta = (ClampMin = "0.01"))
	float NoiseScaleMedium = 3.f;

	/** Macro terrain detail (Mountains and continents). */
	UPROPERTY(EditAnywhere, Category = "Materials|Noise", meta = (ClampMin = "0.01"))
	float NoiseScaleLarge = 100.f;


	/** Initializes default components and basic structure. */
	ACosmicPlanet();

#if WITH_EDITOR
	/** Initial construction logic for visualization in Editor. */
	virtual void OnConstruction(const FTransform& Transform) override;
#endif

	/**
	 * Complete planet configuration.
	 * Used to initialize all properties from a manager or Blueprint.
	 */
	void InitPlanet(
		float InRadiusKm,
		UCosmicNoiseClass* NewNoiseClass,
		FColor Color1, FColor Color2, FColor ColorCold, FColor ColorHot,
		FColor ColorSlope, float ScaleL, float ScaleM, float ScaleS,
		UMaterialInstance* BaseMaterial,
		UTexture2D* DefaultTexture,
		// Clipmap
		bool UseClipmap = true,
		int32 InBaseResolution = 128,
		int32 InNumLevels = 4,
		int32 InMinTriangleSize = 100,
		float InHeightVisibility = 5.0f,
		// Ocean
		bool  bInHasOcean = true,
		double InSeaLevelKm = 0.0,
		int32 InOceanResolution = 128,
		UMaterialInstance* InOceanMaterial = nullptr,
		// Foliage
		UCosmicFoliageCollection* InFoliageCollection = nullptr
	);

	/** Configures behavior and spawn radii of procedural foliage. */
	void SetFoliageParams(
		int32 InFoliageInstancesPerFrame = 50.f,
		float NearLayerRadiusKm = 0.05f,
		float MediumLayerRadiusKm = 0.2f,
		float FarLayerRadiusKm = 0.5f);

	/** Frees memory of noise objects if they are not persistent assets. */
	void CleanupNoiseSettings();

protected:
	/** Startup logic when the game executes. */
	virtual void BeginPlay() override;

#if WITH_EDITOR
	/** Handles actor duplication in the editor ensuring components regenerate. */
	virtual void PostDuplicate(EDuplicateMode::Type Mode) override;
#endif

	/** Cleanup when destroying the actor. */
	virtual void Destroyed() override;

	/** Initial phase of object destruction. */
	virtual void BeginDestroy() override;

	/** Data initialization after all subcomponents are created. */
	virtual void PostInitializeComponents() override;

	/** World execution termination. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Configures and launches Clipmap system generation. */
	void InitClipmap();

	/** Forces total reconstruction of all planetary systems. */
	void RebuildPlanet();

	/** Updates the noise system and its notification delegates. */
	void UpdateNoiseSettings();

	/** Regenerates vegetation and rock systems. */
	void UpdateFoliage();

	/** Synchronizes the ocean component with the current planet radius. */
	void UpdateOcean();

	/** Updates only visual parameters of the material on the terrain. */
	void UpdateMaterialOnly();

	/** Cleans collisions and unbinds active delegates. */
	void ClearData();

	/** Internal flag to avoid redundant reinitializations in the Editor. */
	bool bInitializedInEditor = false;

#if WITH_EDITOR
	/** Details panel change notifier for real-time updates. */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};