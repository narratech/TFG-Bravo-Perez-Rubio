// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CosmicPlanet.generated.h"

class UCosmicClipmapComponent;
class UCosmicNoiseClass;
class UCosmicFoliageSpawner;
class UCosmicCollisionComponent;
class UCosmicPlanetCollisionManager;
class ICosmicNoiseStrategy;
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
	Input, Actor, LOD, Activation, Cooking,
	Physics, Navigation, Tags, DataLayers, LevelInstance))
	class COSMICARCHITECTRUNTIME_API ACosmicPlanet : public AActor
{
	GENERATED_BODY()

public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Called on clients when replicated planet configuration is received. */
	UFUNCTION()
	void OnRep_PlanetConfig();

	/** Base planet radius in Kilometers (supports Large World Coordinates). */
	UPROPERTY(ReplicatedUsing = OnRep_PlanetConfig, EditAnywhere, BlueprintReadOnly, Category = "Planet")
	double RadiusKm = 1.0;

	/** Root component of the actor hierarchy. */
	UPROPERTY(VisibleAnywhere, Category = "Planet", BlueprintReadOnly)
	USceneComponent* Root;

	/** Terrain management system based on concentric levels of detail (Clipmap). */
	UPROPERTY(VisibleAnywhere, Category = "Planet", BlueprintReadOnly)
	TObjectPtr<UCosmicClipmapComponent> ClipmapComponent;

	/** Manages multi-player and relevance-based physical collision patches on planet surface. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet")
	TObjectPtr<UCosmicPlanetCollisionManager> CollisionManager;

	/** Component responsible for visual and physical representation of sea level. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet")
	UCosmicOceanComponent* OceanComponent;

	/** Asset defining noise algorithms for terrain relief. */
	UPROPERTY(ReplicatedUsing = OnRep_PlanetConfig, EditAnywhere, BlueprintReadWrite, Category = "Planet|Noise")
	UCosmicNoiseClass* NoiseClass;

	/** Mass instantiation system for vegetation and rocks on the surface. */
	UPROPERTY(VisibleAnywhere, Category = "Planet", BlueprintReadOnly)
	UCosmicFoliageSpawner* FoliageSpawnerComponent;

	// --- MATERIAL COLOR CONFIGURATION ---

	/** Predominant color for mid-altitude zones. */
	UPROPERTY(ReplicatedUsing = OnRep_PlanetConfig, EditAnywhere, Category = "Materials|Color")
	FColor PlanetMainColor1 = FColor::Red;

	/** Secondary color for terrain chromatic variation. */
	UPROPERTY(ReplicatedUsing = OnRep_PlanetConfig, EditAnywhere, Category = "Materials|Color")
	FColor PlanetMainColor2 = FColor::Orange;

	/** Tint applied to low temperature areas or deep valleys. */
	UPROPERTY(ReplicatedUsing = OnRep_PlanetConfig, EditAnywhere, Category = "Materials|Color")
	FColor PlanetColdColor = FColor::White;

	/** Tint applied to peaks or high activity/temperature areas. */
	UPROPERTY(ReplicatedUsing = OnRep_PlanetConfig, EditAnywhere, Category = "Materials|Color")
	FColor PlanetHotColor = FColor::Red;

	/** Color used to highlight steep slopes and cliffs. */
	UPROPERTY(ReplicatedUsing = OnRep_PlanetConfig, EditAnywhere, Category = "Materials|Color")
	FColor PlanetSlopeColor = FColor::Black;

	// --- NOISE SCALES ---

	/** Fine terrain detail (Micro-relief). */
	UPROPERTY(ReplicatedUsing = OnRep_PlanetConfig, EditAnywhere, Category = "Materials|Noise", meta = (ClampMin = "0.01"))
	float NoiseScaleSmall = 1.f;

	/** Medium terrain detail (Hills and formations). */
	UPROPERTY(ReplicatedUsing = OnRep_PlanetConfig, EditAnywhere, Category = "Materials|Noise", meta = (ClampMin = "0.01"))
	float NoiseScaleMedium = 3.f;

	/** Macro terrain detail (Mountains and continents). */
	UPROPERTY(ReplicatedUsing = OnRep_PlanetConfig, EditAnywhere, Category = "Materials|Noise", meta = (ClampMin = "0.01"))
	float NoiseScaleLarge = 100.f;

	// --- REPLICATED GENERATION CONFIGURATION ---

	UPROPERTY(ReplicatedUsing = OnRep_PlanetConfig)
	UMaterialInstance* BaseMaterial = nullptr;

	UPROPERTY(ReplicatedUsing = OnRep_PlanetConfig)
	UTexture2D* DefaultTexture = nullptr;

	UPROPERTY(ReplicatedUsing = OnRep_PlanetConfig)
	bool bUseClipmap = true;

	UPROPERTY(ReplicatedUsing = OnRep_PlanetConfig)
	int32 BaseResolution = 128;

	UPROPERTY(ReplicatedUsing = OnRep_PlanetConfig)
	int32 NumLevels = 4;

	UPROPERTY(ReplicatedUsing = OnRep_PlanetConfig)
	int32 MinTriangleSize = 100;

	UPROPERTY(ReplicatedUsing = OnRep_PlanetConfig)
	float HeightVisibility = 5.0f;

	UPROPERTY(ReplicatedUsing = OnRep_PlanetConfig)
	bool bHasOcean = true;

	UPROPERTY(ReplicatedUsing = OnRep_PlanetConfig)
	double SeaLevelKm = 0.0;

	UPROPERTY(ReplicatedUsing = OnRep_PlanetConfig)
	int32 OceanResolution = 128;

	UPROPERTY(ReplicatedUsing = OnRep_PlanetConfig)
	UMaterialInstance* OceanMaterial = nullptr;

	UPROPERTY(ReplicatedUsing = OnRep_PlanetConfig)
	UCosmicFoliageCollection* FoliageCollection = nullptr;

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
		UMaterialInstance* InBaseMaterial,
		UTexture2D* InDefaultTexture,
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

	/** Returns the active procedural noise strategy shared across terrain systems. */
	TSharedPtr<ICosmicNoiseStrategy> GetNoiseStrategy();

	/** Recreates or updates the procedural noise strategy from NoiseClass. */
	void UpdateNoiseStrategy();

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

	/** Shared procedural noise strategy for visual terrain clipmaps and physics collision */
	TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy;

#if WITH_EDITOR
	/** Details panel change notifier for real-time updates. */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};